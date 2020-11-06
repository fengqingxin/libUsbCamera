#ifndef __UVC_Property_Range_HPP__
#define __UVC_Property_Range_HPP__

///////////////////////////////// PropertyRange /////////////////////////////////
class PropertyRange {
 public:
  PropertyRange(): start(0), end(0) {}
  PropertyRange(int _start, int _end)
    : start(_start), end(_end) {}

  int size() const {
    return end - start;
  }

  bool empty() const {
    return start == end;
  }

  static PropertyRange all() {
    return PropertyRange(INT_MIN, INT_MAX);
  }

  int start, end;
};

static inline
bool operator == (const PropertyRange& r1, const PropertyRange& r2) {
  return r1.start == r2.start && r1.end == r2.end;
}

static inline
bool operator != (const PropertyRange& r1, const PropertyRange& r2) {
  return !(r1 == r2);
}

static inline
bool operator !(const PropertyRange& r) {
  return r.start == r.end;
}

static inline
PropertyRange operator & (const PropertyRange& r1, const PropertyRange& r2) {
  PropertyRange r(std::max(r1.start, r2.start), std::min(r1.end, r2.end));
  r.end = std::max(r.end, r.start);
  return r;
}

static inline
PropertyRange& operator &= (PropertyRange& r1, const PropertyRange& r2) {
  r1 = r1 & r2;
  return r1;
}

static inline
PropertyRange operator + (const PropertyRange& r1, int delta) {
  return PropertyRange(r1.start + delta, r1.end + delta);
}

static inline
PropertyRange operator + (int delta, const PropertyRange& r1) {
  return PropertyRange(r1.start + delta, r1.end + delta);
}

static inline
PropertyRange operator - (const PropertyRange& r1, int delta) {
  return r1 + (-delta);
}

#endif