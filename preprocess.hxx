/*
 * ClusterRows
 * Copyright (c) 2021 Dennis Francis <dennisfrancis.in@gmail.com>
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

#pragma once

#include "datatypes.hxx"

#include <sal/types.h>
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <vector>

namespace preprocess
{
void getColTypes(
    const com::sun::star::uno::Sequence<com::sun::star::uno::Sequence<com::sun::star::uno::Any>>&
        rData,
    std::vector<DataType>& rColType, std::vector<std::vector<sal_Int32>>& rCol2BlankRowIdx);

void flagEmptyEntries(com::sun::star::uno::Sequence<
                          com::sun::star::uno::Sequence<com::sun::star::uno::Any>>& rDataArray,
                      const std::vector<DataType>& rColType,
                      const std::vector<std::vector<sal_Int32>>& rCol2BlankRowIdx);

void imputeAllColumns(
    com::sun::star::uno::Sequence<com::sun::star::uno::Sequence<com::sun::star::uno::Any>>&
        rDataArray,
    std::vector<DataType>& rColType, const std::vector<std::vector<sal_Int32>>& rCol2BlankRowIdx);

bool imputeWithMode(
    com::sun::star::uno::Sequence<com::sun::star::uno::Sequence<com::sun::star::uno::Any>>&
        rDataArray,
    const sal_Int32 nColIdx, const DataType aType, const std::vector<sal_Int32>& rEmptyRowIndices);

bool imputeWithMedian(
    com::sun::star::uno::Sequence<com::sun::star::uno::Sequence<com::sun::star::uno::Any>>&
        rDataArray,
    const sal_Int32 nColIdx, const DataType aType, const std::vector<sal_Int32>& rEmptyRowIndices);

void calculateFeatureScales(
    com::sun::star::uno::Sequence<com::sun::star::uno::Sequence<com::sun::star::uno::Any>>&
        rDataArray,
    const std::vector<DataType>& rColType, std::vector<std::pair<double, double>>& rFeatureScales);

}