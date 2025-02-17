// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
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

#ifndef EIGEN_DIAGONALPRODUCT_H
#define EIGEN_DIAGONALPRODUCT_H

template<typename LhsNested, typename RhsNested>
struct ei_traits<Product<LhsNested, RhsNested, DiagonalProduct> >
{
  // clean the nested types:
  typedef typename ei_unconst<typename ei_unref<LhsNested>::type>::type _LhsNested;
  typedef typename ei_unconst<typename ei_unref<RhsNested>::type>::type _RhsNested;
  typedef typename _LhsNested::Scalar Scalar;

  enum {
    LhsFlags = _LhsNested::Flags,
    RhsFlags = _RhsNested::Flags,
    RowsAtCompileTime = _LhsNested::RowsAtCompileTime,
    ColsAtCompileTime = _RhsNested::ColsAtCompileTime,
    MaxRowsAtCompileTime = _LhsNested::MaxRowsAtCompileTime,
    MaxColsAtCompileTime = _RhsNested::MaxColsAtCompileTime,

    LhsIsDiagonal = (_LhsNested::Flags&Diagonal)==Diagonal,
    RhsIsDiagonal = (_RhsNested::Flags&Diagonal)==Diagonal,

    CanVectorizeRhs =  (!RhsIsDiagonal) && (RhsFlags & RowMajorBit) && (RhsFlags & PacketAccessBit)
                     && (ColsAtCompileTime % ei_packet_traits<Scalar>::size == 0),

    CanVectorizeLhs =  (!LhsIsDiagonal) && (!(LhsFlags & RowMajorBit)) && (LhsFlags & PacketAccessBit)
                     && (RowsAtCompileTime % ei_packet_traits<Scalar>::size == 0),

    RemovedBits = ~((RhsFlags & RowMajorBit) && (!CanVectorizeLhs) ? 0 : RowMajorBit),

    Flags = ((unsigned int)(LhsFlags | RhsFlags) & HereditaryBits & RemovedBits)
          | (CanVectorizeLhs || CanVectorizeRhs ? PacketAccessBit : 0),

    CoeffReadCost = NumTraits<Scalar>::MulCost + _LhsNested::CoeffReadCost + _RhsNested::CoeffReadCost
  };
};

template<typename LhsNested, typename RhsNested> class Product<LhsNested, RhsNested, DiagonalProduct> : ei_no_assignment_operator,
  public MatrixBase<Product<LhsNested, RhsNested, DiagonalProduct> >
{
    typedef typename ei_traits<Product>::_LhsNested _LhsNested;
    typedef typename ei_traits<Product>::_RhsNested _RhsNested;

    enum {
      RhsIsDiagonal = (_RhsNested::Flags&Diagonal)==Diagonal
    };

  public:

    EIGEN_GENERIC_PUBLIC_INTERFACE(Product)

    template<typename Lhs, typename Rhs>
    inline Product(const Lhs& lhs, const Rhs& rhs)
      : m_lhs(lhs), m_rhs(rhs)
    {
      ei_assert(lhs.cols() == rhs.rows());
    }

    inline int rows() const { return m_lhs.rows(); }
    inline int cols() const { return m_rhs.cols(); }

    const Scalar coeff(int row, int col) const
    {
      const int unique = RhsIsDiagonal ? col : row;
      return m_lhs.coeff(row, unique) * m_rhs.coeff(unique, col);
    }

    template<int LoadMode>
    const PacketScalar packet(int row, int col) const
    {
      if (RhsIsDiagonal)
      {
        ei_assert((_LhsNested::Flags&RowMajorBit)==0);
        return ei_pmul(m_lhs.template packet<LoadMode>(row, col), ei_pset1(m_rhs.coeff(col, col)));
      }
      else
      {
        ei_assert(_RhsNested::Flags&RowMajorBit);
        return ei_pmul(ei_pset1(m_lhs.coeff(row, row)), m_rhs.template packet<LoadMode>(row, col));
      }
    }

  protected:
    const LhsNested m_lhs;
    const RhsNested m_rhs;
};

#endif // EIGEN_DIAGONALPRODUCT_H
