#ifndef __CLUSTERROWS_PREPROCESS__
#define __CLUSTERROWS_PREPROCESS__

#include "datatypes.hxx"

#include <cppu/unotype.hxx>
#include <unordered_set>
#include <vector>
#include <math.h>

#define EMPTYSTRING OUString("__NA__")
#define EMPTYDOUBLE -9999999.0

using com::sun::star::uno::Sequence;
using com::sun::star::uno::Any;


void flagEmptyEntries( Sequence< Sequence< Any > >& rDataArray,
		       const std::vector<DataType>& rColType,
		       const std::vector< std::vector< sal_Int32 > >& rCol2BlankRowIdx );

void imputeAllColumns( Sequence< Sequence< Any > >& rDataArray,
		       std::vector<DataType>& rColType,
		       const std::vector< std::vector< sal_Int32 > >& rCol2BlankRowIdx );

bool imputeWithMode( Sequence< Sequence< Any > >& rDataArray,
		     const sal_Int32 nColIdx,
		     const DataType aType,
		     const std::vector< sal_Int32 >& rEmptyRowIndices );


bool imputeWithMedian( Sequence< Sequence< Any > >& rDataArray,
		       const sal_Int32 nColIdx,
		       const DataType aType,
		       const std::vector< sal_Int32 >& rEmptyRowIndices );

void calculateFeatureScales( Sequence< Sequence< Any > >& rDataArray,
			     const std::vector<DataType>& rColType,
			     std::vector< std::pair< double, double > >& rFeatureScales );



void flagEmptyEntries( Sequence< Sequence< Any > >& rDataArray,
		       const std::vector<DataType>& rColType,
		       const std::vector< std::vector< sal_Int32 > >& rCol2BlankRowIdx )
{
    sal_Int32 nNumCols = rColType.size();
    for ( sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx )
    {
	for ( sal_Int32 nRowIdx : rCol2BlankRowIdx[nColIdx] )
	{
	    if ( rColType[nColIdx] == STRING )
		rDataArray[nRowIdx][nColIdx] <<= EMPTYSTRING;
	    else
		rDataArray[nRowIdx][nColIdx] <<= EMPTYDOUBLE;
	}
    }
}

void imputeAllColumns( Sequence< Sequence< Any > >& rDataArray,
		       std::vector<DataType>& rColType,
		       const std::vector< std::vector< sal_Int32 > >& rCol2BlankRowIdx )
{
    sal_Int32 nNumCols = rColType.size();
    for ( sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx )
    {
	if ( rColType[nColIdx] == STRING )
	    imputeWithMode( rDataArray, nColIdx, rColType[nColIdx], rCol2BlankRowIdx[nColIdx] );
	else if ( rColType[nColIdx] == DOUBLE )
	    imputeWithMedian( rDataArray, nColIdx, rColType[nColIdx], rCol2BlankRowIdx[nColIdx] );
	else if ( rColType[nColIdx] == INTEGER )
	{
	    if ( !imputeWithMode( rDataArray, nColIdx, rColType[nColIdx], rCol2BlankRowIdx[nColIdx] ) )
	    {
		// Better to treat the numbers as continuous rather than discrete classes.
		//rColType[nColIdx] = DOUBLE;
		imputeWithMedian( rDataArray, nColIdx, rColType[nColIdx], rCol2BlankRowIdx[nColIdx] );
	    }
	}
    }
}

bool imputeWithMode( Sequence< Sequence< Any > >& rDataArray,
		     const sal_Int32 nColIdx,
		     const DataType aType,
		     const std::vector< sal_Int32 >& rEmptyRowIndices )
{
    std::unordered_multiset<OUString, OUStringHash> aStringMultiSet;
    std::unordered_multiset<double>                 aDoubleMultiSet;
    OUString aImputeString;
    double   fImputeDouble;
    sal_Int32 nMaxCount = 0;
    sal_Int32 nNumRows = rDataArray.getLength();
    for ( sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx )
    {
	Any aElement = rDataArray[nRowIdx][nColIdx];
	if ( ( aType == STRING && aElement == EMPTYSTRING ) ||
	     ( aType == DOUBLE && aElement == EMPTYDOUBLE ) )
	    continue;

	sal_Int32 nCount = 0;
	if ( aType == STRING )
	{
	    OUString aStr;
	    aElement >>= aStr;
	    aStringMultiSet.insert( aStr );
	    nCount = aStringMultiSet.count( aStr );
	}
	else
	{
	    double fVal;
	    aElement >>= fVal;
	    aDoubleMultiSet.insert( fVal );
	    nCount = aDoubleMultiSet.count( fVal );
	}
	if ( nCount > nMaxCount )
	{
	    if ( aType == STRING )
		aElement >>= aImputeString;
	    else
		aElement >>= fImputeDouble;

	    nMaxCount = nCount;
	}
    }

    bool bGood = true;
    if ( aType == INTEGER )
    {
	if ( nMaxCount < 3 ) // Ensure at least 3 samples of top class
	    bGood = false;
    }

    if ( bGood )
    {
	if ( aType == STRING )
	    for ( sal_Int32 nMissingIdx : rEmptyRowIndices )
		rDataArray[nMissingIdx][nColIdx] <<= aImputeString;
	else
	    for ( sal_Int32 nMissingIdx : rEmptyRowIndices )
		rDataArray[nMissingIdx][nColIdx] <<= fImputeDouble;
    }

    return bGood;
}


bool imputeWithMedian( Sequence< Sequence< Any > >& rDataArray,
		       const sal_Int32 nColIdx,
		       const DataType aType,
		       const std::vector< sal_Int32 >& rEmptyRowIndices )
{
    // We are sure that this function is not called for Any == OUString
    assert( aType != STRING && "imputeWithMedian called with type OUString !!!" );

    sal_Int32 nNumRows = rDataArray.getLength();
    sal_Int32 nNumEmptyElements = rEmptyRowIndices.size();
    std::vector<double> aCopy( nNumRows );
    for ( sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx )
	rDataArray[nRowIdx][nColIdx] >>= aCopy[nRowIdx];

    std::sort( aCopy.begin(), aCopy.end() );
    size_t nElements = nNumRows - nNumEmptyElements;
    double fMedian;

    if ( ( nElements % 2 ) == 0 )
    {
	double fMed1 = aCopy[nNumEmptyElements + (nElements/2)];
	double fMed2 = aCopy[nNumEmptyElements + (nElements/2) - 1];
	fMedian = 0.5*( fMed1 + fMed2 );
    }
    else
	fMedian = aCopy[nNumEmptyElements + (nElements/2)];
    
    for ( sal_Int32 nMissingIdx : rEmptyRowIndices )
	rDataArray[nMissingIdx][nColIdx] <<= fMedian;

    return true;
}

void calculateFeatureScales( Sequence< Sequence< Any > >& rDataArray,
			     const std::vector<DataType>& rColType,
			     std::vector< std::pair< double, double > >& rFeatureScales )
{
    sal_Int32 nNumRows = rDataArray.getLength();
    sal_Int32 nNumCols = rColType.size();

    for ( sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx )
    {
	if ( rColType[nColIdx] == STRING )
	    continue;
	double fSum = 0.0, fSum2 = 0.0;
	for ( sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx )
	{
	    double fVal;
	    rDataArray[nRowIdx][nColIdx] >>= fVal;
	    fSum  += fVal;
	    fSum2 += (fVal*fVal);
	}
	double fMean = fSum / static_cast<double>(nNumRows);
	double fStd  = sqrt( ( fSum2 / static_cast<double>(nNumRows) ) - ( fMean*fMean ) );
	rFeatureScales[nColIdx].first  = fMean;
	// Avoid 0 standard deviation condition.
	rFeatureScales[nColIdx].second = ( fStd == 0.0 ) ? fMean : fStd;
    }
}

#endif
