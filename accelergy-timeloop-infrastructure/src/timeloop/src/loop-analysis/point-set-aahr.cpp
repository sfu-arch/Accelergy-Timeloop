/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include "loop-analysis/point-set.hpp"

// ---------------------------------------------
//                   Gradient
// ---------------------------------------------

Gradient::Gradient(std::uint32_t _order) :
    order(_order)
{
  Reset();
}

void Gradient::Reset()
{
  dimension = 0;
  value = 0;
}
  
std::int32_t Gradient::Sign() const
{
  if (value < 0)
  {
    return -1;
  }
  else if (value == 0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
  
void Gradient::Print(std::ostream& out) const
{
  out << "< ";
  for (unsigned i = 0; i < order; i++)
  {
    if (i == dimension)
      out << value << " ";
    else
      out << "0 ";
  }
  out << ">";
}

std::ostream& operator << (std::ostream& out, const Gradient& g)
{
  out << "< ";
  for (unsigned i = 0; i < g.order; i++)
  {
    if (i == g.dimension)
      out << g.value << " ";
    else
      out << "0 ";
  }
  out << ">";
  return out;
}

// ---------------------------------------------
//        AAHR Point Set implementation
// ---------------------------------------------

AxisAlignedHyperRectangle::AxisAlignedHyperRectangle(std::uint32_t order) :
    order_(order),
    min_(order),
    max_(order),
    gradient_(order)
{
  Reset();
}

AxisAlignedHyperRectangle::AxisAlignedHyperRectangle(std::uint32_t order, const Point unit) :
    AxisAlignedHyperRectangle(order)
{
  ASSERT(order_ == unit.Order());
  min_ = unit;
  for (unsigned dim = 0; dim < order_; dim++)
  {
    max_[dim] = min_[dim] + 1;
  }
}

AxisAlignedHyperRectangle::AxisAlignedHyperRectangle(std::uint32_t order, const Point min, const Point max) :
    AxisAlignedHyperRectangle(order)
{
  min_ = min;
  max_ = max;
}

AxisAlignedHyperRectangle::AxisAlignedHyperRectangle(std::uint32_t order, const std::vector<std::pair<Point, Point>> corner_sets) :
    AxisAlignedHyperRectangle(order)
{
  ASSERT(corner_sets.size() == 1);
  min_ = corner_sets.front().first;
  max_ = corner_sets.front().second;
}

AxisAlignedHyperRectangle::AxisAlignedHyperRectangle(const AxisAlignedHyperRectangle& a) :
    order_(a.order_),
    min_(a.min_),
    max_(a.max_),
    gradient_(a.gradient_)
{
}

// Copy-and-swap idiom.
AxisAlignedHyperRectangle& AxisAlignedHyperRectangle::operator = (AxisAlignedHyperRectangle other)
{
  swap(*this, other);
  return *this;
}

void swap(AxisAlignedHyperRectangle& first, AxisAlignedHyperRectangle& second)
{
  using std::swap;
  swap(first.order_, second.order_);
  swap(first.min_, second.min_);
  swap(first.max_, second.max_);
  swap(first.gradient_, second.gradient_);
}

Point AxisAlignedHyperRectangle::Min() const
{
  return min_;
}

Point AxisAlignedHyperRectangle::Max() const
{
  return max_;
}

std::size_t AxisAlignedHyperRectangle::size() const
{
  std::size_t size = max_[0] - min_[0];
  for (unsigned i = 1; i < order_; i++)
  {
    size *= (max_[i] - min_[i]);
  }
  return size;
}

bool AxisAlignedHyperRectangle::empty() const
{
  return (size() == 0);
}

void AxisAlignedHyperRectangle::Reset()
{
  min_.Reset();
  max_.Reset();
  gradient_.Reset();
}

void AxisAlignedHyperRectangle::Add(const Point& p, bool extrude_if_discontiguous)
{
  Add(AxisAlignedHyperRectangle(order_, p), extrude_if_discontiguous);
}

void AxisAlignedHyperRectangle::ExtrudeAdd(const AxisAlignedHyperRectangle& s)
{
  Add(s, true);
}

void AxisAlignedHyperRectangle::Add(const AxisAlignedHyperRectangle& s, bool extrude_if_discontiguous)
{
  ASSERT(order_ == s.order_);
    
  // Special cases.
  if (size() == 0)
  {
    *this = s;
    return;
  }

  if (s.size() == 0)
  {
    return;
  }

  if (*this == s)
  {
    return;
  }

  auto orig = *this;
    
  // Both AAHRs should have identical min_, max_ along all-but-one axes, and
  // must be contiguous along the but-one axis.
  bool found = false;
  for (unsigned dim = 0; dim < order_; dim++)
  {
    if (s.max_[dim] >= min_[dim] && max_[dim] >= s.min_[dim])
    {
      auto u = *this;
      bool need_update = false;
        
      if (s.min_[dim] < min_[dim])
      {
        u.min_[dim] = s.min_[dim];
        need_update = true;
      }
      if (s.max_[dim] > max_[dim])
      {
        u.max_[dim] = s.max_[dim];
        need_update = true;
      }
        
      if (need_update)
      {
        if (found)
        {
          std::cout << "AAHR Add error: non-HR shape\n";
          orig.Print(); std::cout << std::endl;
          s.Print(); std::cout << std::endl;          
          assert(false);
        }
        else
        {
          *this = u;
          found = true;
        }
      }
    }
    else
    {
      if (!extrude_if_discontiguous)
      {
        std::cout << "AAHR Add error: discontiguous volumes (and extrude is disabled)\n";
        orig.Print(); std::cout << std::endl;
        s.Print(); std::cout << std::endl;          
        assert(false);
      }
      else
      {
        auto u = *this;
        bool need_update = false;
        
        if (s.max_[dim] < min_[dim])
        {
          u.min_[dim] = s.min_[dim];
          need_update = true;
        }
        else
        {
          u.max_[dim] = s.max_[dim];
          need_update = true;
        }
        
        if (need_update)
        {
          if (found)
          {
            std::cout << "AAHR Add error: non-HR shape\n";
            orig.Print(); std::cout << std::endl;
            s.Print(); std::cout << std::endl;          
            assert(false);
          }
          else
          {
            *this = u;
            found = true;
          }
        }
      } // extrude_if_discontiguous
    }
  }
}

Gradient AxisAlignedHyperRectangle::Subtract(const AxisAlignedHyperRectangle& s)
{
  ASSERT(order_ == s.order_);
    
  // Special cases.
  if (size() == 0 || s.size() == 0)
  {
    return Gradient(order_);
  }

  if (*this == s)
  {
    Reset();
    return Gradient(order_);
  }

  for (unsigned dim = 0; dim < order_; dim++)
  {
    if (s.max_[dim] <= min_[dim] || s.min_[dim] >= max_[dim])
    {
      // No overlap along even a single dimension means there's
      // no intersection at all. Skip this function.
      return Gradient(order_);
    }
  }
 
  auto updated = *this;
  Gradient gradient(order_);
    
  // General case: Both AAHRs should have identical min_, max_ along
  // all-but-one axes, and be contiguous or overlapping along the but-one
  // axis. If this isn't true, then torpedo everything, keep the source
  // as the result, and set gradient to 0.
  bool found = false;
  for (unsigned dim = 0; dim < order_; dim++)
  {
    if (min_[dim] != s.min_[dim] || max_[dim] != s.max_[dim])
    {
      if (found)
      {
        // Torpedo everything, set delta to source, gradient to 0.
        // WARNING: this simply discards potential non-AAHR shapes,
        // which is something we do want to do occasionally. However,
        // there may be bugs causing non-AAHR shapes, which will be
        // masked by this step.
        return Gradient(order_);
      }
        
      found = true;

      if (s.min_[dim] <= min_[dim])
      {
        if (s.max_[dim] <= max_[dim])
        {
          gradient.dimension = dim;
          gradient.value = s.max_[dim] - min_[dim];
          updated.min_[dim] = s.max_[dim];
        }
        else
        {
          gradient.Reset();
          updated.max_[dim] = min_[dim];
        }
      }
      else if (s.min_[dim] > min_[dim])
      {
        if (s.max_[dim] < max_[dim])
        {
          assert(false);

          // The accuracy of the following comment is questionable. Fractures
          // don't appear to be happening for the dataflows we've been looking
          // at so far. We are enabling the assertion.
            
          // Subtraction is causing a fracture. This can happen during
          // macro tile changes with sliding windows. Discard the operand,
          // and return a zero gradient.
          return Gradient(order_);
        }
        else
        {
          gradient.dimension = dim;
          gradient.value = s.min_[dim] - max_[dim];
          updated.max_[dim] = s.min_[dim];
        }
      }
      else
      {
        assert(false);
      }

      // If we just shrunk the AAHR down to NULL, reset it into canonical form
      // and skip the remainder of this function.
      if (updated.min_[dim] == updated.max_[dim])
      {
        // Discard updated, we're going to Reset ourselves anyway.
        Reset();
        return Gradient(order_);
      }
    }
  }

  assert(found);

  *this = updated;
    
  return gradient;
}

AxisAlignedHyperRectangle& AxisAlignedHyperRectangle::operator += (const Point& p)
{
  Add(p, true); // true => always extrude.
  return *this;
}

AxisAlignedHyperRectangle& AxisAlignedHyperRectangle::operator += (const AxisAlignedHyperRectangle& s)
{
  Add(s, true); // true => always extrude.
  return *this;
}

AxisAlignedHyperRectangle AxisAlignedHyperRectangle::operator - (const AxisAlignedHyperRectangle& s)
{
  // Calculate the delta.
  AxisAlignedHyperRectangle delta(*this);

  if (gResetOnStrideChange)
  {
    auto g = delta.Subtract(s);
    
    // Now check if the newly-calculated gradient is different from the gradient
    // of the operand. UGH, this is ugly. This code shouldn't be in the math
    // library, it should be outside.
    if (s.gradient_.value == 0)
    {
      // Gradient was zero. Use newly-computed gradient.
      gradient_ = g;
    }
    else if (g.value == 0 && delta.size() == 0)
    {
      // Note the delta size check. We need that because the gradient can
      // be zero in two cases:
      // - The set difference really yielded a 0 (the case we're capturing here).
      // - There was no intersection and therefore gradient was invalid (we'll
      //   default to the final else.
      // FIXME: UGH UGH UGH.
      gradient_ = g;
    }
    else if (s.gradient_.dimension == g.dimension &&
             s.gradient_.Sign() == g.Sign())
    {
      // New gradient is in the same direction as current gradient.
      gradient_ = g;
    }
    else
    {
      // New gradient is either in a different dimension, or a different
      // direction (+/-) in the same dimension. Discard my residual state,
      // and re-initialize gradient.
      delta = *this;
      gradient_ = Gradient(order_);
    }
  }
  else
  {
    delta.Subtract(s);
  }
    
  // The delta itself doesn't carry a gradient.
  delta.gradient_ = Gradient(order_);

  return delta;
}

std::vector<AxisAlignedHyperRectangle> AxisAlignedHyperRectangle::MultiSubtract(const AxisAlignedHyperRectangle& b)
{
  // Quick check: if there's no overlap in even a single rank, return a.
  for (unsigned rank = 0; rank < order_; rank++)
  {
    if (max_[rank] <= b.min_[rank] || b.max_[rank] <= min_[rank])
      return { *this };
  }

  // There's an intersection.
  std::vector<AxisAlignedHyperRectangle> retval;

  AxisAlignedHyperRectangle middle(*this);

  for (unsigned rank = 0; rank < order_; rank++)
  {
    // Left slice.
    if (middle.min_[rank] < b.min_[rank])
    {
      AxisAlignedHyperRectangle left(middle);
      left.max_[rank] = b.min_[rank];                
      retval.push_back(left);

      // Advance middle.min_ to discard the slice we just created.
      middle.min_[rank] = b.min_[rank];
    }

    // Right slice.
    if (b.max_[rank] < middle.max_[rank])
    {
      AxisAlignedHyperRectangle right(middle);
      right.min_[rank] = b.max_[rank];                
      retval.push_back(right);

      // Regress middle.max_ to discard the slice we just created.
      middle.max_[rank] = b.max_[rank];
    }
  }

  // if (retval.size() > 1)
  // {
  //   std::cout << "a: "; this->Print(); std::cout << std::endl;
  //   std::cout << "b: "; b.Print(); std::cout << std::endl;
  //   std::cout << "diffs:\n";
  //   for (auto& x: retval)
  //   {
  //     std::cout << "  "; x.Print(); std::cout << std::endl;
  //   }

  //   std::cout << "Replaying...\n";

  //   std::vector<AxisAlignedHyperRectangle> retval;
  //   AxisAlignedHyperRectangle middle(*this);

  //   for (unsigned rank = 0; rank < order_; rank++)
  //   {
  //     std::cout << "BEGIN rank " << rank << std::endl;
  //     // Left slice.
  //     if (middle.min_[rank] < b.min_[rank])
  //     {
  //       AxisAlignedHyperRectangle left(middle);
  //       left.max_[rank] = b.min_[rank];
  //       std::cout << "  adding left slice: "; left.Print(); std::cout << std::endl;
  //       retval.push_back(left);

  //       // Advance middle.min_ to discard the slice we just created.
  //       std::cout << "  advancing middle min from " << middle.min_[rank] << " to " << b.min_[rank] << std::endl;
  //       middle.min_[rank] = b.min_[rank];
  //     }

  //     // Right slice.
  //     if (b.max_[rank] < middle.max_[rank])
  //     {
  //       AxisAlignedHyperRectangle right(middle);
  //       right.min_[rank] = b.max_[rank];                
  //       std::cout << "  adding right slice: "; right.Print(); std::cout << std::endl;
  //       retval.push_back(right);

  //       // Regress middle.max_ to discard the slice we just created.
  //       std::cout << "  regressing middle max from " << middle.max_[rank] << " to " << b.max_[rank] << std::endl;
  //       middle.max_[rank] = b.max_[rank];
  //     }
  //   }
  // }

  return retval;    
}

bool AxisAlignedHyperRectangle::Contains(const Point& p) const
{
  ASSERT(p.Order() == order_);

  for (unsigned rank = 0; rank < order_; rank++)
  {
    if (p[rank] < min_[rank] || p[rank] >= max_[rank])
    {
      return false;
    }
  }

  return true;
}

bool AxisAlignedHyperRectangle::MergeIfAdjacent(const Point& p)
{
  ASSERT(p.Order() == order_);

  if (empty())
  {
    min_ = p;
    max_ = p;
    max_.IncrementAllDimensions();
    return true;
  }

  // This only works for an AAHR that exists only along 1 rank.
  bool success = true;
  bool match = false;
  unsigned matching_rank;

  for (unsigned rank = 0; rank < order_; rank++)
  {
    if (p[rank] == min_[rank]-1 || p[rank] == max_[rank])
    {
      // Matching rank found.
      if (match)
      {
        // Oops, cannot match twice.
        success = false;
        break;
      }
      else
      {
        match = true;
        matching_rank = rank;
      }
    }
    else if (p[rank] == min_[rank] && min_[rank]+1 == max_[rank])
    {
      // All good, p is aligned with AAHR.
    }
    else
    {
      success = false;
      break;
    }
  }

  success &= match;

  if (success)
  {
    if (p[matching_rank] == min_[matching_rank]-1)
    {
      min_[matching_rank]--;
    }
    else if (p[matching_rank] == max_[matching_rank])
    {
      max_[matching_rank]++;
    }
  }

  return success;
}

bool AxisAlignedHyperRectangle::operator == (const AxisAlignedHyperRectangle& s) const
{
  ASSERT(order_ == s.order_);
    
  for (unsigned dim = 0; dim < order_; dim++)
  {
    if (min_[dim] != s.min_[dim] || max_[dim] != s.max_[dim])
    {
      return false;
    }
  }
  return true;
}

std::vector<double> AxisAlignedHyperRectangle::Centroid() const
{
  std::vector<double> centroid(order_);
  for (unsigned rank = 0; rank < order_; rank++)
  {
    centroid[rank] = min_[rank] + double(max_[rank] - 1 - min_[rank]) / 2;
  }
  return centroid;
}

Point AxisAlignedHyperRectangle::GetTranslation(const AxisAlignedHyperRectangle& s) const
{
  ASSERT(order_ == s.order_);

  Point vector(order_);

  for (unsigned dim = 0; dim < order_; dim++)
  {
    auto min_delta = s.min_[dim] - min_[dim];
    auto max_delta = s.max_[dim] - max_[dim];

    // Both AAHRs should have the same shape for this operation to be legal.
    ASSERT(min_delta == max_delta);

    vector[dim] = min_delta;
  }    
    
  return vector;
}

void AxisAlignedHyperRectangle::Translate(const Point& p)
{
  ASSERT(order_ == p.Order());

  for (unsigned dim = 0; dim < order_; dim++)
  {
    min_[dim] += p[dim];
    max_[dim] += p[dim];
  }    
}

void AxisAlignedHyperRectangle::Print(std::ostream& out) const
{
  out << "["; 
  for (unsigned dim = 0; dim < order_-1; dim++)
  {
    out << min_[dim] << ",";
  }
  out << min_[order_-1];
  out << ":";
  for (unsigned dim = 0; dim < order_-1; dim++)
  {
    out << max_[dim] << ",";
  }
  out << max_[order_-1];
  out << ")";
  // out << " gradient = ";
  // gradient_.Print(out);
}

std::ostream& operator << (std::ostream& out, const AxisAlignedHyperRectangle& x)
{
  out << "["; 
  for (unsigned dim = 0; dim < x.order_-1; dim++)
  {
    out << x.min_[dim] << ",";
  }
  out << x.min_[x.order_-1];
  out << ":";
  for (unsigned dim = 0; dim < x.order_-1; dim++)
  {
    out << x.max_[dim] << ",";
  }
  out << x.max_[x.order_-1];
  out << ")";
  // out << " gradient = ";
  // gradient_.Print(out);
  return out;
}
