// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2006-2008 Benoit Jacob <jacob@math.jussieu.fr>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#ifndef EIGEN_DIAGONALMATRIX_H
#define EIGEN_DIAGONALMATRIX_H

/** \class DiagonalMatrix
  *
  * \brief Expression of a diagonal matrix
  *
  * \param CoeffsVectorType the type of the vector of diagonal coefficients
  *
  * This class is an expression of a diagonal matrix with given vector of diagonal
  * coefficients. It is the return
  * type of MatrixBase::diagonal(const OtherDerived&) and most of the time this is
  * the only way it is used.
  *
  * \sa MatrixBase::diagonal(const OtherDerived&)
  */
template<typename CoeffsVectorType>
struct ei_traits<DiagonalMatrix<CoeffsVectorType> >
{
  typedef typename CoeffsVectorType::Scalar Scalar;
  typedef typename ei_nested<CoeffsVectorType>::type CoeffsVectorTypeNested;
  typedef typename ei_unref<CoeffsVectorTypeNested>::type _CoeffsVectorTypeNested;
  enum {
    RowsAtCompileTime = CoeffsVectorType::SizeAtCompileTime,
    ColsAtCompileTime = CoeffsVectorType::SizeAtCompileTime,
    MaxRowsAtCompileTime = CoeffsVectorType::MaxSizeAtCompileTime,
    MaxColsAtCompileTime = CoeffsVectorType::MaxSizeAtCompileTime,
    Flags = (_CoeffsVectorTypeNested::Flags & HereditaryBits) | Diagonal,
    CoeffReadCost = _CoeffsVectorTypeNested::CoeffReadCost
  };
};

template<typename CoeffsVectorType>
class DiagonalMatrix : ei_no_assignment_operator,
   public MatrixBase<DiagonalMatrix<CoeffsVectorType> >
{
  public:

    EIGEN_GENERIC_PUBLIC_INTERFACE(DiagonalMatrix)

    inline DiagonalMatrix(const CoeffsVectorType& coeffs) : m_coeffs(coeffs)
    {
      ei_assert(CoeffsVectorType::IsVectorAtCompileTime
          && coeffs.size() > 0);
    }

    inline int rows() const { return m_coeffs.size(); }
    inline int cols() const { return m_coeffs.size(); }

    inline const Scalar coeff(int row, int col) const
    {
      return row == col ? m_coeffs.coeff(row) : static_cast<Scalar>(0);
    }

  protected:
    const typename CoeffsVectorType::Nested m_coeffs;
};

/** \returns an expression of a diagonal matrix with *this as vector of diagonal coefficients
  *
  * \only_for_vectors
  *
  * \addexample AsDiagonalExample \label How to build a diagonal matrix from a vector
  *
  * Example: \include MatrixBase_asDiagonal.cpp
  * Output: \verbinclude MatrixBase_asDiagonal.out
  *
  * \sa class DiagonalMatrix, isDiagonal()
  **/
template<typename Derived>
inline const DiagonalMatrix<Derived>
MatrixBase<Derived>::asDiagonal() const
{
  return derived();
}

/** \returns true if *this is approximately equal to a diagonal matrix,
  *          within the precision given by \a prec.
  *
  * Example: \include MatrixBase_isDiagonal.cpp
  * Output: \verbinclude MatrixBase_isDiagonal.out
  *
  * \sa asDiagonal()
  */
template<typename Derived>
bool MatrixBase<Derived>::isDiagonal
(RealScalar prec) const
{
  if(cols() != rows()) return false;
  RealScalar maxAbsOnDiagonal = static_cast<RealScalar>(-1);
  for(int j = 0; j < cols(); j++)
  {
    RealScalar absOnDiagonal = ei_abs(coeff(j,j));
    if(absOnDiagonal > maxAbsOnDiagonal) maxAbsOnDiagonal = absOnDiagonal;
  }
  for(int j = 0; j < cols(); j++)
    for(int i = 0; i < j; i++)
    {
      if(!ei_isMuchSmallerThan(coeff(i, j), maxAbsOnDiagonal, prec)) return false;
      if(!ei_isMuchSmallerThan(coeff(j, i), maxAbsOnDiagonal, prec)) return false;
    }
  return true;
}

#endif // EIGEN_DIAGONALMATRIX_H
