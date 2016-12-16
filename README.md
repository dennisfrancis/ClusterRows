# ClusterRows

A LibreOffice Calc extension that clusters the rows in a table and color them to indicate the clusters.

To install the prebuilt extension, use the Extension Manager of LibreOffice and browse to this repo's file ClusterRows.oxt. Alternatively you can install the extension using terminal as :
```
$ git clone https://github.com/dennisfrancis/ClusterRows.git
$ cd ClusterRows
$ unopkg install ./ClusterRows.oxt
```

To use this extension on some data in a sheet in LibreOffice, place the cursor on any cell inside your table with data ( no need to select the whole table ) and go to the menu Clustering, and click on "Cluster rows and color them". The rows are colored with respect to the cluster assignment. Two new columns [ClusterId and Confidence] are added to the right of the table. **ClusterId** specifies the cluster to which the row is assigned and **Confidence** indicates the algorithm's confidence in scale [0,1] that this cluster assignment may be correct (higher number means higher confidence).

The project uses an in-house implementation of [Expectation Maximization](http://cs229.stanford.edu/notes/cs229-notes7b.pdf) algorithm to compute the clusters. It chooses the number of clusters parameter via [Bayesian information criterion](https://en.wikipedia.org/wiki/Bayesian_information_criterion).

The project is under heavy development, but is usable. Whole of the project was written from scratch and it does not depend on any Machine Learning or Linear Algebra library. Full source code is made available under [GPL3 license](https://www.gnu.org/licenses/gpl-3.0.en.html). If you are interested in understanding how to build LibreOffice extensions like this, I have a blog for that at [https://niocs.github.io/LOBook/extensions/index.html](https://niocs.github.io/LOBook/extensions/index.html).

A major issue is that the prebuilt extension (.oxt file) supports only modern GNU/Linux 64 bit systems comparable to Fedora 24. However support for MS Windows and MacOSX is planned. Another caveat is that for the extension to work, at least 10 samples(rows) are needed in the table.

## Building the extension from source

First you need to setup LibreOffice SDK as per the instruction in [http://api.libreoffice.org/docs/install.html](http://api.libreoffice.org/docs/install.html).
Then do the below :

```
$ cd ClusterRows
$ make
```

If you get errors in compilation, please check if the SDK's environment variables are set properly after setting up the SDK. If that does not solve it, open an issue here.
If you are compiling in a GNU/Linux platform and used the standard defaults while setting up the SDK, the oxt file can be found at the location
`/home/$username/libreoffice5.4_sdk/LINUXexample.out/bin/`

# Sample usage of the extension
In `testdocs` directory there is a spreadsheet file called `three-clusters.ods`. In that file there is a dataset artificially created from a 3-cluster Gausian mixture model in the range `A1:C301`.
This dataset has just two dimensions/variables(column A and B). Column C has the ground truth cluster id information of each row. To test the extension a copy of the dataset is made(excluding the ground-truth column) to `F1:G301`. Now place the cursor anywhere in the range `F1:G301` and go to the menu `Clustering > Find 3 clusters`. The extension will compute the 3 clusters and produce the columns `ClusterId` and `Confidence`. The cell `L9` now will indicate the clustering accuracy. This is a measure of how well the clustering algorithm was able to assign clusters compared to the ground truths in the original dataset. Typically we get around 97% accuracy for this dataset.
![Clustering Output](img/output.png)
The document also produces visualizations of the data in sheet `charts`. The left chart shows the data points colored according to the ground truth clusters of the dataset. The middle chart shows the data points without the ground truth information, which is provided as input to the clustering algorithm. The right chart shows the data colored as per the cluster assignments made by the clustering algorithm.
![Visualization](img/chart.png)

As always pull-requests are welcome.
