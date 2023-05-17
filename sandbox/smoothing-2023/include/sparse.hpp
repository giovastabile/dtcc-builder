#ifndef SPARSE_H
#define SPARSE_H

#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

class COO_array
{
private:
  size_t _row_num;
  size_t _col_num;
  size_t _nnz;

public:
  uint shape[2];
  std::vector<uint> row;
  std::vector<uint> col;
  std::vector<double> data;

  COO_array(size_t n, size_t m);

  COO_array(size_t n, size_t m, double *denseArray);

  COO_array(size_t n, size_t m, size_t nnz, double *denseArray);

  ~COO_array();

  size_t nnz();

  void info();

  void add(uint rowIndex, uint colIndex, double value);

  std::vector<double> toarray();
};

COO_array::COO_array(size_t n, size_t m)
{
  this->_row_num = n;
  this->_col_num = m;
  this->shape[0] = n;
  this->shape[1] = m;
  this->_nnz = 0;
}

COO_array::COO_array(size_t n, size_t m, double *denseArray)
{
  this->_row_num = n;
  this->_col_num = m;
  this->shape[0] = n;
  this->shape[1] = m;
  this->_nnz = 0;

  for (size_t i = 0; i < n * m; i++)
  {
    if (denseArray[i] != 0)
      this->_nnz++;
  }

  this->col.reserve(this->_nnz);
  this->row.reserve(this->_nnz);
  this->data.reserve(this->_nnz);

  for (size_t i = 0; i < n; i++)
  {
    for (size_t j = 0; j < m; j++)
    {
      if (denseArray[i * n + j] != 0)
      {
        this->row.push_back(i);
        this->col.push_back(j);
        this->data.push_back(denseArray[i * n + j]);
      }
    }
  }
}

COO_array::COO_array(size_t n, size_t m, size_t nnz, double *denseArray)
{
  this->_row_num = n;
  this->_col_num = m;
  this->shape[0] = n;
  this->shape[1] = m;
  this->_nnz = nnz;

  this->col.reserve(this->_nnz);
  this->row.reserve(this->_nnz);
  this->data.reserve(this->_nnz);

  for (size_t i = 0; i < n; i++)
  {
    for (size_t j = 0; j < m; j++)
    {
      if (denseArray[i * n + j] != 0)
      {
        this->row.push_back(i);
        this->col.push_back(j);
        this->data.push_back(denseArray[i * n + j]);
      }
    }
  }
}

COO_array::~COO_array() {}

// Add non-zero element to the COO Array. (Not the best immplementation. Really
// slow for very large Matrices)
void COO_array::add(uint rowIndex, uint colIndex, double value)
{
  if (value == 0)
    return;

  if (rowIndex >= this->_row_num || colIndex >= this->_col_num)
  {
    std::cout << "Coordinate Indexes out of bounds" << std::endl;
    return;
  }

  if ((this->row.empty()) || (rowIndex > this->row[this->row.size() - 1] &&
                              colIndex > this->col[this->col.size() - 1]))
  {
    this->row.push_back(rowIndex);
    this->col.push_back(colIndex);
    this->data.push_back(value);
    this->_nnz++;
  }
  else
  {

    const auto p =
        std::equal_range(this->row.begin(), this->row.end(), rowIndex);
    const auto index_of_first = p.first - this->row.begin();
    const auto index_of_last = p.second - this->row.begin();

    for (size_t i = index_of_first; i < index_of_last; i++)
    {
      if (colIndex == col[i])
      {
        data[i] += value;
        return;
      }
    }

    const auto first = next(this->col.begin(), index_of_first);
    const auto last = next(this->col.begin(), index_of_last);

    auto col_pos_it = upper_bound(first, last, colIndex);
    auto pos = col_pos_it - this->col.begin();
    auto row_pos_it = next(row.begin(), pos);
    auto val_pos_it = next(data.begin(), pos);

    this->col.insert(col_pos_it, colIndex);
    this->row.insert(row_pos_it, rowIndex);
    this->data.insert(val_pos_it, value);
    this->_nnz++;
  }
}

std::vector<double> COO_array::toarray()
{
  std::vector<double> denseArray(this->shape[0] * this->shape[1], 0);

  for (size_t i = 0; i < this->row.size(); i++)
  {
    denseArray[row[i] * this->shape[0] + col[i]] = data[i];
  }

  return denseArray;
}

size_t COO_array::nnz() { return this->_nnz; }

void COO_array::info()
{
  std::cout << "Num of Rows: " << this->shape[0] << std::endl;
  std::cout << "Num of Cols: " << this->shape[1] << std::endl;
  std::cout << "Num of NonZero: " << _nnz << std::endl;

  std::cout << "Data:\t Size:\t" << this->data.size();
  // for (size_t i = 0; i < this->data.size(); i++)
  // {
  //    std::cout << this->data[i] << "\t";
  // }
  std::cout << "\nRow Index: \tSize:\t" << this->data.size();
  // for (size_t i = 0; i < this->row.size(); i++)
  //  {
  //     std::cout << this->row[i] << "\t";
  //  }
  std::cout << "\nCol Index: \tSize:\t" << this->data.size();
  // for (size_t i = 0; i < this->col.size(); i++)
  // {
  //    std::cout << this->col[i] << "\t";
  // }
  std::cout << std::endl << std::endl;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CSR SPARSE MATRIX FORMAT CLASS
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class CSR_array
{
private:
  size_t _row_num;
  size_t _col_num;
  size_t _nnz;

public:
  uint shape[2];
  std::vector<uint> colIdx;
  std::vector<uint> rowPtr;
  std::vector<double> data;

  CSR_array(size_t n, size_t m);

  CSR_array(size_t n, size_t m, double *denseArray);

  CSR_array(COO_array *fromCOO);

  ~CSR_array();

  void toarray(std::vector<double> &denseArray);

  void info();
};

CSR_array::CSR_array(size_t n, size_t m)
{
  this->_row_num = n;
  this->_col_num = m;
  this->shape[0] = n;
  this->shape[1] = m;
  this->rowPtr.reserve(m + 1);
  this->_nnz = 0;
}

CSR_array::CSR_array(size_t m, size_t n, double *denseArray)
{
  this->_row_num = m;
  this->_col_num = n;
  this->shape[0] = m;
  this->shape[1] = n;
  this->rowPtr.reserve(m + 1);
  this->_nnz = 0;

  for (size_t i = 0; i < n * m; i++)
  {
    if (denseArray[i] != 0)
      this->_nnz++;
  }

  this->data.reserve(_nnz);
  this->colIdx.reserve(_nnz);
  this->rowPtr.reserve(m + 1);

  rowPtr.push_back(0); // First element of Row ptr is always zero
  for (size_t i = 0; i < m; i++)
  {
    rowPtr.push_back(rowPtr[i]); // i+1

    for (size_t j = 0; j < n; j++)
    {
      if (denseArray[i * m + j] != 0)
      {
        data.push_back(denseArray[i * m + j]);
        colIdx.push_back(j);
        rowPtr[i + 1]++;
      }
    }
  }
}
CSR_array::~CSR_array() {}

void CSR_array::info()
{
  std::cout << "Num of Rows: " << this->shape[0] << std::endl;
  std::cout << "Num of Cols: " << this->shape[1] << std::endl;
  std::cout << "Num of NonZero: " << _nnz << std::endl;

  std::cout << "Data:\t Size:\t" << this->data.size();
  // for (size_t i = 0; i < this->data.size(); i++)
  // {
  //    std::cout << this->data[i] << "\t";
  // }
  std::cout << "\nCol Index: \tSize:\t" << this->colIdx.size();
  // for (size_t i = 0; i < this->colIdx.size(); i++)
  // {
  //    std::cout << this->colIdx[i] << "\t";
  // }
  std::cout << "\nRow Pointer: \tSize:\t" << this->rowPtr.size();
  // for (size_t i = 0; i < this->rowPtr.size(); i++)
  // {
  //    std::cout << this->rowPtr[i] << "\t";
  // }
  std::cout << std::endl << std::endl;
}

CSR_array::CSR_array(COO_array *coo)
{
  this->shape[0] = coo->shape[0];
  this->shape[1] = coo->shape[1];

  this->_nnz = coo->nnz();

  this->colIdx.reserve(_nnz);
  this->data.reserve(_nnz);
  this->rowPtr.resize(coo->shape[0] + 1);
  std::fill(this->rowPtr.begin(), this->rowPtr.end(), 0);

  size_t at_row = 0;
  for (size_t i = 0; i < coo->data.size(); i++)
  {
    this->colIdx.push_back(coo->col[i]);
    this->data.push_back(coo->data[i]);
    this->rowPtr[coo->row[i] + 1]++;
  }
  for (size_t i = 0; i < this->shape[0]; i++)
  {
    this->rowPtr[i + 1] += this->rowPtr[i];
  }
}

void CSR_array::toarray(std::vector<double> &denseArray)
{

  denseArray.resize(this->shape[0] * this->shape[1]);
  std::fill(denseArray.begin(), denseArray.end(), 0);
  for (size_t i = 0; i < this->shape[0]; i++)
  {
    for (size_t k = this->rowPtr[i]; k < this->rowPtr[i + 1]; k++)
    {
      denseArray[i * this->shape[0] + this->colIdx[k]] = this->data[k];
    }
  }
}

#endif