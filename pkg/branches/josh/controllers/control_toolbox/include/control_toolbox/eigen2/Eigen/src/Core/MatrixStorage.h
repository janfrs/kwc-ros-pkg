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

#ifndef EIGEN_MATRIXSTORAGE_H
#define EIGEN_MATRIXSTORAGE_H

/** \internal
  *
  * \class ei_matrix_storage
  *
  * \brief Stores the data of a matrix
  *
  * This class stores the data of fixed-size, dynamic-size or mixed matrices
  * in a way as compact as possible.
  *
  * \sa Matrix
  */
template<typename T, int Size, int _Rows, int _Cols> class ei_matrix_storage;

// purely fixed-size matrix
template<typename T, int Size, int _Rows, int _Cols> class ei_matrix_storage
{
    ei_aligned_array<T,Size,((Size*sizeof(T))%16)==0> m_data;
  public:
    inline ei_matrix_storage() {}
    inline ei_matrix_storage(int,int,int) {}
    inline void swap(ei_matrix_storage& other) { std::swap(m_data,other.m_data); }
    inline static int rows(void) {return _Rows;}
    inline static int cols(void) {return _Cols;}
    inline void resize(int,int,int) {}
    inline const T *data() const { return m_data.array; }
    inline T *data() { return m_data.array; }
};

// dynamic-size matrix with fixed-size storage
template<typename T, int Size> class ei_matrix_storage<T, Size, Dynamic, Dynamic>
{
    ei_aligned_array<T,Size,((Size*sizeof(T))%16)==0> m_data;
    int m_rows;
    int m_cols;
  public:
    inline ei_matrix_storage(int, int rows, int cols) : m_rows(rows), m_cols(cols) {}
    inline ~ei_matrix_storage() {}
    inline void swap(ei_matrix_storage& other)
    { std::swap(m_data,other.m_data); std::swap(m_rows,other.m_rows); std::swap(m_cols,other.m_cols); }
    inline int rows(void) const {return m_rows;}
    inline int cols(void) const {return m_cols;}
    inline void resize(int, int rows, int cols)
    {
      m_rows = rows;
      m_cols = cols;
    }
    inline const T *data() const { return m_data.array; }
    inline T *data() { return m_data.array; }
};

// dynamic-size matrix with fixed-size storage and fixed width
template<typename T, int Size, int _Cols> class ei_matrix_storage<T, Size, Dynamic, _Cols>
{
    ei_aligned_array<T,Size,((Size*sizeof(T))%16)==0> m_data;
    int m_rows;
  public:
    inline ei_matrix_storage(int, int rows, int) : m_rows(rows) {}
    inline ~ei_matrix_storage() {}
    inline void swap(ei_matrix_storage& other) { std::swap(m_data,other.m_data); std::swap(m_rows,other.m_rows); }
    inline int rows(void) const {return m_rows;}
    inline int cols(void) const {return _Cols;}
    inline void resize(int /*size*/, int rows, int)
    {
      m_rows = rows;
    }
    inline const T *data() const { return m_data.array; }
    inline T *data() { return m_data.array; }
};

// dynamic-size matrix with fixed-size storage and fixed height
template<typename T, int Size, int _Rows> class ei_matrix_storage<T, Size, _Rows, Dynamic>
{
    ei_aligned_array<T,Size,((Size*sizeof(T))%16)==0> m_data;
    int m_cols;
  public:
    inline ei_matrix_storage(int, int, int cols) : m_cols(cols) {}
    inline ~ei_matrix_storage() {}
    inline void swap(ei_matrix_storage& other) { std::swap(m_data,other.m_data); std::swap(m_cols,other.m_cols); }
    inline int rows(void) const {return _Rows;}
    inline int cols(void) const {return m_cols;}
    inline void resize(int size, int, int cols)
    {
      m_cols = cols;
    }
    inline const T *data() const { return m_data.array; }
    inline T *data() { return m_data.array; }
};

// purely dynamic matrix.
template<typename T> class ei_matrix_storage<T, Dynamic, Dynamic, Dynamic>
{
    T *m_data;
    int m_rows;
    int m_cols;
  public:
    inline ei_matrix_storage(int size, int rows, int cols)
      : m_data(ei_aligned_malloc<T>(size)), m_rows(rows), m_cols(cols) {}
    inline ~ei_matrix_storage() { ei_aligned_free(m_data); }
    inline void swap(ei_matrix_storage& other)
    { std::swap(m_data,other.m_data); std::swap(m_rows,other.m_rows); std::swap(m_cols,other.m_cols); }
    inline int rows(void) const {return m_rows;}
    inline int cols(void) const {return m_cols;}
    void resize(int size, int rows, int cols)
    {
      if(size != m_rows*m_cols)
      {
        ei_aligned_free(m_data);
        m_data = ei_aligned_malloc<T>(size);
      }
      m_rows = rows;
      m_cols = cols;
    }
    inline const T *data() const { return m_data; }
    inline T *data() { return m_data; }
};

// matrix with dynamic width and fixed height (so that matrix has dynamic size).
template<typename T, int _Rows> class ei_matrix_storage<T, Dynamic, _Rows, Dynamic>
{
    T *m_data;
    int m_cols;
  public:
    inline ei_matrix_storage(int size, int, int cols) : m_data(ei_aligned_malloc<T>(size)), m_cols(cols) {}
    inline ~ei_matrix_storage() { ei_aligned_free(m_data); }
    inline void swap(ei_matrix_storage& other) { std::swap(m_data,other.m_data); std::swap(m_cols,other.m_cols); }
    inline static int rows(void) {return _Rows;}
    inline int cols(void) const {return m_cols;}
    void resize(int size, int, int cols)
    {
      if(size != _Rows*m_cols)
      {
        ei_aligned_free(m_data);
        m_data = ei_aligned_malloc<T>(size);
      }
      m_cols = cols;
    }
    inline const T *data() const { return m_data; }
    inline T *data() { return m_data; }
};

// matrix with dynamic height and fixed width (so that matrix has dynamic size).
template<typename T, int _Cols> class ei_matrix_storage<T, Dynamic, Dynamic, _Cols>
{
    T *m_data;
    int m_rows;
  public:
    inline ei_matrix_storage(int size, int rows, int) : m_data(ei_aligned_malloc<T>(size)), m_rows(rows) {}
    inline ~ei_matrix_storage() { ei_aligned_free(m_data); }
    inline void swap(ei_matrix_storage& other) { std::swap(m_data,other.m_data); std::swap(m_rows,other.m_rows); }
    inline int rows(void) const {return m_rows;}
    inline static int cols(void) {return _Cols;}
    void resize(int size, int rows, int)
    {
      if(size != m_rows*_Cols)
      {
        ei_aligned_free(m_data);
        m_data = ei_aligned_malloc<T>(size);
      }
      m_rows = rows;
    }
    inline const T *data() const { return m_data; }
    inline T *data() { return m_data; }
};

#endif // EIGEN_MATRIX_H
