#include "VectorDouble.hpp"
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <cassert>
#include <iostream>
#include <math.h>

//VectorDouble::VectorDouble()
//{
//    mSize = -1;
//}

VectorDouble::VectorDouble(int Size)
{
    assert(Size > 0);
    using namespace boost::numeric::ublas;
    
    mSize = Size;
    
    switch (Size)
    {
        case 1:
            mpVectorOf1 = new c_vector<double,1>(1); 
            break;
        case 2:
            mpVectorOf2 = new c_vector<double,2> (2); 
            break; 
        case 3:
            mpVectorOf3 = new c_vector<double,3> (3); 
            break; 
        case 4:
            mpVectorOf4 = new c_vector<double,4> (4); 
            break; 
        
        default:
        // Vector biger than  4 Throw Exception
        throw("Vector size larger than 4");
            break;
    }
}

VectorDouble::VectorDouble(const VectorDouble& rOtherVector)
{
    mSize = rOtherVector.mSize;
    
    switch (mSize)
    {
        case 1:
             mpVectorOf1 = new c_vector<double,1>(*(rOtherVector.mpVectorOf1)); 
             break;
        case 2:
             mpVectorOf2= new c_vector<double,2>(*(rOtherVector.mpVectorOf2)); 
             break; 
        case 3:
             mpVectorOf3 = new c_vector<double,3>(*(rOtherVector.mpVectorOf3));         
             break; 
        case 4:
             mpVectorOf4 = new c_vector<double,4>(*(rOtherVector.mpVectorOf4));         
             break; 
        
        default:
        // Vector biger than  4 Throw Exception
        throw("Vector size larger than 4");
            break;
    }
    
    
}

VectorDouble::~VectorDouble()
{
    switch (mSize)
    {
        case 1:
            delete (mpVectorOf1);
            break;
        case 2:
            delete (mpVectorOf2);
            break;
        case 3:
            delete (mpVectorOf3);
            break;
        case 4:
            delete (mpVectorOf4);
            break;
        default:
        // Vector biger than  4 Throw Exception
        throw("Vector size larger than 4");
            break;
    }        
}    
    
double& VectorDouble::operator()(int entry) const
{
    assert(entry > -1);
    assert(entry < mSize);
    
    switch(mSize) {
        case 1:
            return (*mpVectorOf1) (entry);
        case 2:
            return (*mpVectorOf2) (entry); 
        case 3:
            return (*mpVectorOf3) (entry);   
        case 4:
            return (*mpVectorOf4) (entry);   
        default:
            // Vector biger than  4 Throw Exception
            throw("Vector size larger than 4");
    }

}

    
VectorDouble& VectorDouble::operator=(const VectorDouble& rOtherVector)
{
    assert( mSize == rOtherVector.mSize);
    switch(mSize) {
        case 1:
            *mpVectorOf1=*(rOtherVector.mpVectorOf1);
            break;
        case 2:
            *mpVectorOf2=*(rOtherVector.mpVectorOf2);
            break;
        case 3:
            *mpVectorOf3=*(rOtherVector.mpVectorOf3);
            break;
        case 4:
            *mpVectorOf4=*(rOtherVector.mpVectorOf4);
            break;
        default:
            // Vector biger than  4 Throw Exception
            throw("Vector size larger than 4");
    }
           
}
   


VectorDouble VectorDouble::operator+(const VectorDouble& rSomeVector)
{
    assert(mSize==rSomeVector.mSize);
    VectorDouble result(mSize);
    
    switch(mSize) {
        case 1:
            *(result.mpVectorOf1)=*mpVectorOf1 + *(rSomeVector.mpVectorOf1);
            break;
        case 2:
            *(result.mpVectorOf2)=*mpVectorOf2 + *(rSomeVector.mpVectorOf2);
            break;
        case 3:
            *(result.mpVectorOf3)=*mpVectorOf3 + *(rSomeVector.mpVectorOf3);
            break;
        case 4:
            *(result.mpVectorOf4)=*mpVectorOf4 + *(rSomeVector.mpVectorOf4);
            break;
        default:
            // Vector biger than  4 Throw Exception
            throw("Vector size larger than 4");
    }
    return result;
}


VectorDouble VectorDouble::operator-(const VectorDouble& rSomeVector)
{
    assert(mSize==rSomeVector.mSize);
    VectorDouble result(mSize);
    
    switch(mSize) {
        case 1:
            *(result.mpVectorOf1)=*mpVectorOf1 - *(rSomeVector.mpVectorOf1);
            break;
        case 2:
            *(result.mpVectorOf2)=*mpVectorOf2 - *(rSomeVector.mpVectorOf2);
            break;
        case 3:
            *(result.mpVectorOf3)=*mpVectorOf3 - *(rSomeVector.mpVectorOf3);
            break;
        case 4:
            *(result.mpVectorOf4)=*mpVectorOf4 - *(rSomeVector.mpVectorOf4);
            break;
        default:
            // Vector biger than  4 Throw Exception
            throw("Vector size larger than 4");
    }
    return result;
}


int VectorDouble::Size(void) const
{
    return mSize;
}

double VectorDouble::dot(const VectorDouble& rSomeVector) const
{
    assert(mSize==rSomeVector.mSize);
    
    switch(mSize) {
        case 1:
            return inner_prod(*(mpVectorOf1), *rSomeVector.mpVectorOf1);
        case 2:
            return inner_prod(*(mpVectorOf2), *rSomeVector.mpVectorOf2);
        case 3:
            return inner_prod(*(mpVectorOf3), *rSomeVector.mpVectorOf3);
        case 4:
            return inner_prod(*(mpVectorOf4), *rSomeVector.mpVectorOf4);
        default:
            // Vector biger than  4 Throw Exception
            throw("Vector size larger than 4");
    }
}

        
void VectorDouble::ResetToZero( void )
{
    switch(mSize) {
        case 1:
           *(mpVectorOf1) = zero_vector<double>(1);
           break;
        case 2:
           *(mpVectorOf2) = zero_vector<double>(2);
           break;
        case 3:
           *(mpVectorOf3) = zero_vector<double>(3);
           break;
        case 4:
           *(mpVectorOf4) = zero_vector<double>(4);
           break;
        default:
            // Vector biger than  4 Throw Exception
            throw("Vector size larger than 4");
    }
    
}


VectorDouble VectorDouble::VectorProduct(const VectorDouble& rSomeVector)
{
    //This is a cross-product
    //only implemented for 3-vectors
    assert(mSize==3);
    assert(rSomeVector.Size()==3);
    
    VectorDouble result(3);
    
    double x1=(*mpVectorOf3)(0);
    double y1=(*mpVectorOf3)(1);
    double z1=(*mpVectorOf3)(2);
    double x2=rSomeVector(0);
    double y2=rSomeVector(1);
    double z2=rSomeVector(2);
    
    result(0)=y1*z2-z1*y2;
    result(1)=z1*x2-x1*z2;
    result(2)=x1*y2-y1*x2;
    
    return result;
}




VectorDouble VectorDouble::operator*(double scalar)
{
    VectorDouble result(mSize);
    for (int i=0; i<mSize; i++)
    {
        result(i)=scalar * (*this)(i);
    }
    return result;
}

VectorDouble operator*(double scalar, const VectorDouble& rSomeVector)
{
    VectorDouble result(rSomeVector.Size());
    for (int i=0; i<rSomeVector.Size(); i++)
    {
        result(i)=scalar*rSomeVector(i);
    }
    return result;
}


double VectorDouble::L2Norm( void )
{
   switch(mSize) {
        case 1:
           return norm_2(*mpVectorOf1);
        case 2:         
           return norm_2(*mpVectorOf2);
        case 3:
           return norm_2(*mpVectorOf3);
        case 4:
           return norm_2(*mpVectorOf4);
         default:
            // Vector biger than  4 Throw Exception
            throw("Vector size larger than 4");
    }
    
}

c_vector<double, 1> VectorDouble::GetUblasHandle1( void ) const
{
       assert (mSize==1);         
       return *mpVectorOf1;
}

c_vector<double, 2> VectorDouble::GetUblasHandle2( void ) const
{
       assert (mSize==2);
       return *mpVectorOf2;
}

c_vector<double, 3> VectorDouble::GetUblasHandle3( void ) const
{
       assert (mSize==3);
       return *mpVectorOf3;
}

c_vector<double, 4> VectorDouble::GetUblasHandle4( void ) const
{
       assert (mSize==4);
       return *mpVectorOf4;
}


