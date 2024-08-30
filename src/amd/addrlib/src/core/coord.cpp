      /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      // Coordinate class implementation
   #include "addrcommon.h"
   #include "coord.h"
      namespace Addr
   {
   namespace V2
   {
      Coordinate::Coordinate()
   {
      dim = DIM_X;
      }
      Coordinate::Coordinate(enum Dim dim, INT_32 n)
   {
         }
      VOID Coordinate::set(enum Dim d, INT_32 n)
   {
      dim = d;
      }
      UINT_32 Coordinate::ison(const UINT_32 *coords) const
   {
                  }
      enum Dim Coordinate::getdim()
   {
         }
      INT_8 Coordinate::getord()
   {
         }
      BOOL_32 Coordinate::operator==(const Coordinate& b)
   {
         }
      BOOL_32 Coordinate::operator<(const Coordinate& b)
   {
               if (dim == b.dim)
   {
         }
   else
   {
      if (dim == DIM_S || b.dim == DIM_M)
   {
         }
   else if (b.dim == DIM_S || dim == DIM_M)
   {
         }
   else if (ord == b.ord)
   {
         }
   else
   {
                        }
      BOOL_32 Coordinate::operator>(const Coordinate& b)
   {
      BOOL_32 lt = *this < b;
   BOOL_32 eq = *this == b;
      }
      BOOL_32 Coordinate::operator<=(const Coordinate& b)
   {
         }
      BOOL_32 Coordinate::operator>=(const Coordinate& b)
   {
         }
      BOOL_32 Coordinate::operator!=(const Coordinate& b)
   {
         }
      Coordinate& Coordinate::operator++(INT_32)
   {
      ord++;
      }
      // CoordTerm
      CoordTerm::CoordTerm()
   {
         }
      VOID CoordTerm::Clear()
   {
         }
      VOID CoordTerm::add(Coordinate& co)
   {
      // This function adds a coordinate INT_32o the list
   // It will prevent the same coordinate from appearing,
   // and will keep the list ordered from smallest to largest
            for (i = 0; i < num_coords; i++)
   {
      if (m_coord[i] == co)
   {
         }
   if (m_coord[i] > co)
   {
         for (UINT_32 j = num_coords; j > i; j--)
   {
         }
   m_coord[i] = co;
   num_coords++;
               if (i == num_coords)
   {
      m_coord[num_coords] = co;
         }
      VOID CoordTerm::add(CoordTerm& cl)
   {
      for (UINT_32 i = 0; i < cl.num_coords; i++)
   {
            }
      BOOL_32 CoordTerm::remove(Coordinate& co)
   {
      BOOL_32 remove = FALSE;
   for (UINT_32 i = 0; i < num_coords; i++)
   {
      if (m_coord[i] == co)
   {
         remove = TRUE;
            if (remove)
   {
            }
      }
      BOOL_32 CoordTerm::Exists(Coordinate& co)
   {
      BOOL_32 exists = FALSE;
   for (UINT_32 i = 0; i < num_coords; i++)
   {
      if (m_coord[i] == co)
   {
         exists = TRUE;
      }
      }
      VOID CoordTerm::copyto(CoordTerm& cl)
   {
      cl.num_coords = num_coords;
   for (UINT_32 i = 0; i < num_coords; i++)
   {
            }
      UINT_32 CoordTerm::getsize()
   {
         }
      UINT_32 CoordTerm::getxor(const UINT_32 *coords) const
   {
      UINT_32 out = 0;
   for (UINT_32 i = 0; i < num_coords; i++)
   {
         }
      }
      VOID CoordTerm::getsmallest(Coordinate& co)
   {
         }
      UINT_32 CoordTerm::Filter(INT_8 f, Coordinate& co, UINT_32 start, enum Dim axis)
   {
      for (UINT_32 i = start;  i < num_coords;)
   {
      if (((f == '<' && m_coord[i] < co) ||
         (f == '>' && m_coord[i] > co) ||
   (f == '=' && m_coord[i] == co)) &&
   {
         for (UINT_32 j = i; j < num_coords - 1; j++)
   {
         }
   }
   else
   {
            }
      }
      Coordinate& CoordTerm::operator[](UINT_32 i)
   {
         }
      BOOL_32 CoordTerm::operator==(const CoordTerm& b)
   {
               if (num_coords != b.num_coords)
   {
         }
   else
   {
      for (UINT_32 i = 0; i < num_coords; i++)
   {
         // Note: the lists will always be in order, so we can compare the two lists at time
   if (m_coord[i] != b.m_coord[i])
   {
      ret = FALSE;
         }
      }
      BOOL_32 CoordTerm::operator!=(const CoordTerm& b)
   {
         }
      BOOL_32 CoordTerm::exceedRange(const UINT_32 *ranges)
   {
      BOOL_32 exceed = FALSE;
   for (UINT_32 i = 0; (i < num_coords) && (exceed == FALSE); i++)
   {
                     }
      // coordeq
   CoordEq::CoordEq()
   {
         }
      VOID CoordEq::remove(Coordinate& co)
   {
      for (UINT_32 i = 0; i < m_numBits; i++)
   {
            }
      BOOL_32 CoordEq::Exists(Coordinate& co)
   {
               for (UINT_32 i = 0; i < m_numBits; i++)
   {
      if (m_eq[i].Exists(co))
   {
            }
      }
      VOID CoordEq::resize(UINT_32 n)
   {
      if (n > m_numBits)
   {
      for (UINT_32 i = m_numBits; i < n; i++)
   {
            }
      }
      UINT_32 CoordEq::getsize()
   {
         }
      UINT_64 CoordEq::solve(const UINT_32 *coords) const
   {
      UINT_64 out = 0;
   for (UINT_32 i = 0; i < m_numBits; i++)
   {
         }
      }
      VOID CoordEq::solveAddr(
      UINT_64 addr, UINT_32 sliceInM,
      {
                                          for (UINT_32 i = 0; i < temp.m_numBits; i++)
   {
               if (termSize == 1)
   {
         INT_8 bit = (addr >> i) & 1;
                                          }
   else if (termSize > 1)
   {
                     if (bitsLeft > 0)
   {
      if (sliceInM != 0)
   {
         coords[DIM_Z] = coords[DIM_M] / sliceInM;
            do
   {
                  for (UINT_32 i = 0; i < temp.m_numBits; i++)
                     if (termSize == 1)
   {
                                                            }
                           for (UINT_32 j = 0; j < termSize; j++)
                                       if (BitsValid[dim] & (1u << ord))
   {
                                                   }
      VOID CoordEq::copy(CoordEq& o, UINT_32 start, UINT_32 num)
   {
      o.m_numBits = (num == 0xFFFFFFFF) ? m_numBits : num;
   for (UINT_32 i = 0; i < o.m_numBits; i++)
   {
            }
      VOID CoordEq::reverse(UINT_32 start, UINT_32 num)
   {
               for (UINT_32 i = 0; i < n / 2; i++)
   {
      CoordTerm temp;
   m_eq[start + i].copyto(temp);
   m_eq[start + n - 1 - i].copyto(m_eq[start + i]);
         }
      VOID CoordEq::xorin(CoordEq& x, UINT_32 start)
   {
      UINT_32 n = ((m_numBits - start) < x.m_numBits) ? (m_numBits - start) : x.m_numBits;
   for (UINT_32 i = 0; i < n; i++)
   {
            }
      UINT_32 CoordEq::Filter(INT_8 f, Coordinate& co, UINT_32 start, enum Dim axis)
   {
      for (UINT_32 i = start; i < m_numBits;)
   {
      UINT_32 m = m_eq[i].Filter(f, co, 0, axis);
   if (m == 0)
   {
         for (UINT_32 j = i; j < m_numBits - 1; j++)
   {
         }
   }
   else
   {
            }
      }
      VOID CoordEq::shift(INT_32 amount, INT_32 start)
   {
      if (amount != 0)
   {
      INT_32 numBits = static_cast<INT_32>(m_numBits);
   amount = -amount;
   INT_32 inc = (amount < 0) ? -1 : 1;
   INT_32 i = (amount < 0) ? numBits - 1 : start;
   INT_32 end = (amount < 0) ? start - 1 : numBits;
   for (; (inc > 0) ? i < end : i > end; i += inc)
   {
         if ((i + amount < start) || (i + amount >= numBits))
   {
         }
   else
   {
               }
      CoordTerm& CoordEq::operator[](UINT_32 i)
   {
         }
      VOID CoordEq::mort2d(Coordinate& c0, Coordinate& c1, UINT_32 start, UINT_32 end)
   {
      if (end == 0)
   {
      ADDR_ASSERT(m_numBits > 0);
      }
   for (UINT_32 i = start; i <= end; i++)
   {
      UINT_32 select = (i - start) % 2;
   Coordinate& c = (select == 0) ? c0 : c1;
   m_eq[i].add(c);
         }
      VOID CoordEq::mort3d(Coordinate& c0, Coordinate& c1, Coordinate& c2, UINT_32 start, UINT_32 end)
   {
      if (end == 0)
   {
      ADDR_ASSERT(m_numBits > 0);
      }
   for (UINT_32 i = start; i <= end; i++)
   {
      UINT_32 select = (i - start) % 3;
   Coordinate& c = (select == 0) ? c0 : ((select == 1) ? c1 : c2);
   m_eq[i].add(c);
         }
      BOOL_32 CoordEq::operator==(const CoordEq& b)
   {
               if (m_numBits != b.m_numBits)
   {
         }
   else
   {
      for (UINT_32 i = 0; i < m_numBits; i++)
   {
         if (m_eq[i] != b.m_eq[i])
   {
      ret = FALSE;
         }
      }
      BOOL_32 CoordEq::operator!=(const CoordEq& b)
   {
         }
      } // V2
   } // Addr
