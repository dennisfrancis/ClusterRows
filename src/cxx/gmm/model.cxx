/*
* ClusterRows
* Copyright (c) 2023 Dennis Francis <dennisfrancis.in@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cfloat>
#include <em.hxx>
#include <logging.hxx>
#include <gmm/model.hxx>
#include <gmm/cluster.hxx>

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <random>

gmm::Model::Model(const Map<const MatrixXdRM>& data_, int num_clusters_)
    : weights(num_clusters_, data_.rows()) // c x m
    , data(data_)
    , num_clusters(num_clusters_)
{
}

namespace
{

using namespace Eigen;

void init_clusters(std::vector<gmm::Cluster>& clusters, int num_clusters, const gmm::Data& data)
{
    if (static_cast<int>(clusters.size()) != num_clusters)
    {
        clusters.clear();
        clusters.reserve(num_clusters);
        for (int cidx = 0; cidx < num_clusters; ++cidx)
        {
            // std::cerr << "[INFO] \t cluster = " << cidx << '\n';
            clusters.push_back({ cidx, data, num_clusters });
        }
    }

    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::vector<int> sample_indices(data.rows());
    for (int sample = 0; sample < data.rows(); ++sample)
    {
        sample_indices[sample] = sample;
    }

    std::shuffle(sample_indices.begin(), sample_indices.end(), std::default_random_engine(seed));

    for (int cluster = 0; cluster < num_clusters; ++cluster)
    {
        const int sample = sample_indices[cluster];
        clusters[cluster].init(sample);
    }
}

} // anonymous namespace

double gmm::Model::fit(int num_epochs, int num_iterations)
{
    double bic{ 1.0E10 };
    std::vector<gmm::Cluster> epoch_clusters;
    MatrixXd epoch_weights{ num_clusters, data.rows() };
    // data.display();

    for (int epoch = 0; epoch < num_epochs; ++epoch)
    {
        // std::cerr << "[INFO] epoch = " << epoch << " started.\n";
        // No need to initialize weights.
        init_clusters(epoch_clusters, num_clusters, data);
        writeLog("\tEpoch#%d : ", epoch);
        double epoch_bic = run_epoch(num_iterations, epoch_weights, epoch_clusters);
        writeLog("\n\tepoch_bic = %f\n", epoch_bic);

        if (epoch_bic < bic)
        {
            const auto& better_weights{ epoch_weights };
            // Optimization: No need to save epoch_clusters as we can determine
            // best cluster allocation from epoch_weights.
            weights = better_weights; // Uses copy ctor.
            writeLog("Improvement in global bic from %f to %f\n", bic, epoch_bic);
            bic = epoch_bic;
        }
    }

    if (false)
    {
        std::cerr << "Final:\n";
        for (int cluster = 0; cluster < num_clusters; ++cluster)
        {
            const auto& ecluster = epoch_clusters[cluster];
            std::cerr << "Cluster#" << cluster << ":\n";
            std::cerr << "phi = " << ecluster.phi << '\n';
            std::cerr << "mu = \n" << ecluster.mu << '\n';
            std::cerr << "sigma = \n" << ecluster.sigma << '\n';
        }
    }

    return bic;
}

void gmm::Model::get_labels(int* labels, double* confidence_scores) const
{
    if (!labels || !confidence_scores)
    {
        throw std::runtime_error(
            "Model::get_labels: no labels or confidence_scores array to store to.");
    }

    const int m{ samples() };
    const int c{ clusters() };
    for (int sample = 0; sample < m; ++sample)
    {
        double best_confidence{ -1.0 };
        double sum{ 0.0 };
        for (int cluster = 0; cluster < c; ++cluster)
        {
            const double& confidence{ weights(cluster, sample) };
            sum += confidence;
            if (confidence > best_confidence)
            {
                best_confidence = confidence;
                labels[sample] = cluster;
                confidence_scores[sample] = confidence;
            }
        }

        // Normalize the score.
        confidence_scores[sample] /= sum;
    }
}

double gmm::Model::run_epoch(int num_iterations, MatrixXd& epoch_weights,
                             std::vector<gmm::Cluster>& epoch_clusters) const
{
    double epoch_bic{ 1.0E10 };
    for (int iter = 0; iter < num_iterations; ++iter)
    {
        double bic = compute_expectation(epoch_weights, epoch_clusters);
        writeLog("%f ", bic);

        if (bic < epoch_bic && (epoch_bic - bic > EPSILON))
        {
            epoch_bic = bic;
        }
        else
        {
            break;
        }

        // std::cerr << "[INFO] \t\t iter = " << iter << " bic = " << bic << '\n';
        maximize_likelihood(epoch_weights, epoch_clusters);
    }

    writeLog("\n[INFO] Best BIC score of epoch = %f\n", epoch_bic);
    return epoch_bic;
}

double gmm::Model::compute_expectation(MatrixXd& epoch_weights,
                                       const std::vector<gmm::Cluster>& epoch_clusters) const
{
    const int m = samples();
    const int c = clusters();
    double bic = 0.0;
    std::vector<double> normalizers(m, 0.0);
    for (int cluster = 0; cluster < c; ++cluster)
    {
        const auto& ecluster = epoch_clusters[cluster];
        const auto cov_inverse = ecluster.inv_covar_matrix();
        const double cov_determinant = ecluster.cov_determinant();
        for (int sample = 0; sample < m; ++sample)
        {
            double wt = ecluster.sample_probability(sample, cov_inverse, cov_determinant);
            epoch_weights(cluster, sample) = wt;
            normalizers[sample] += wt;
        }
    }

    // std::cerr << "\n[Best cluster allocation] : ";
    for (int sample = 0; sample < m; ++sample)
    {
        const double& normalizer = normalizers[sample];
        // if (sample < 5)
        //     std::cerr << "\n|sample#" << sample << " normalizer = " << normalizer << " :  ";
        double best_cluster_weight{ 0.0 };
        for (int cluster = 0; cluster < c; ++cluster)
        {
            double& wt = epoch_weights(cluster, sample);
            // if (sample < 5)
            //     std::cerr << "cluster#" << cluster << " -> ";
            // if (sample < 5)
            //     std::cerr << " wt = " << wt << "|  ";
            // if (sample == 5)
            //     std::cerr << '\n';
            wt /= normalizer;
            if (wt > best_cluster_weight)
            {
                best_cluster_weight = wt;
            }
        }
        bic += (-std::log(std::abs(best_cluster_weight)));
    }

    return bic;
}

void gmm::Model::maximize_likelihood(const MatrixXd& epoch_weights,
                                     std::vector<Cluster>& epoch_clusters) const
{
    const int m = samples();
    const int n = dims();
    const int c = clusters();

    // update phi and mu. Also initialize sigma to zero matrix.
    for (int cluster = 0; cluster < c; ++cluster)
    {
        auto& ecluster{ epoch_clusters[cluster] };
        ecluster.clear_mu_sigma();
        double cluster_weight{ 0.0 };
        for (int sample = 0; sample < m; ++sample)
        {
            double wt = epoch_weights(cluster, sample);
            cluster_weight += wt;
            ecluster.mu += (wt * data(sample).reshaped(n, 1));
        }
        ecluster.phi = cluster_weight / m;
        //if (cluster_weight > DBL_MIN)
        {
            ecluster.mu /= cluster_weight;
        }
    }

    // Update sigma
    for (int cluster = 0; cluster < c; ++cluster)
    {
        auto& ecluster{ epoch_clusters[cluster] };
        double cluster_weight{ 0.0 };
        for (int sample = 0; sample < m; ++sample)
        {
            double wt = epoch_weights(cluster, sample);
            cluster_weight += wt;

            MatrixXd x = data(sample).reshaped(n, 1);
            ecluster.sigma += (wt * ((x - ecluster.mu) * (x - ecluster.mu).transpose()));
        }

        // if (cluster_weight > DBL_MIN)
        {
            ecluster.sigma /= cluster_weight;
        }
    }
}

gmm::GMM::GMM(const double* data_, int rows_, int cols_, int min_clusters_, int max_clusters_,
              int num_epochs_, int num_iterations_)
    : data{ data_, rows_, cols_ }
    , min_clusters{ min_clusters_ }
    , max_clusters{ max_clusters_ }
    , num_epochs{ num_epochs_ }
    , num_iterations{ num_iterations_ }
{
}

void gmm::GMM::fit()
{
    double best_bic{ 1.0E10 };
    int best_numclusters = min_clusters;
    for (int clusters = min_clusters; clusters <= max_clusters; ++clusters)
    {
        writeLog("\nFitting for #clusters = %d\n", clusters);
        auto model{ std::make_unique<Model>(data, clusters) };
        double bic = model->fit(num_epochs, num_iterations);
        if (bic < best_bic)
        {
            best_model = std::move(model);
            best_bic = bic;
            best_numclusters = clusters;
        }
    }

    writeLog("\nBest model BIC score = %f, num clusters = %d\n", best_bic, best_numclusters);
}

void gmm::GMM::get_labels(int* labels, double* confidence_scores) const
{
    if (!best_model)
    {
        throw std::runtime_error("GMM::get_labels: no model found");
    }

    best_model->get_labels(labels, confidence_scores);
}
