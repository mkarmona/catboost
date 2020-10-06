#pragma once

#include <util/generic/map.h>
#include <util/system/yassert.h>

#include <type_traits>

template <class T>
class TDisjointIntervalTree {
private:
    static_assert(std::is_integral<T>::value, "expect std::is_integral<T>::value");

    using TTree = TMap<T, T>; // [key, value)
    using TIterator = typename TTree::iterator;
    using TConstIterator = typename TTree::const_iterator;
    using TReverseIterator = typename TTree::reverse_iterator;
    using TThis = TDisjointIntervalTree<T>;

    TTree Tree;
    size_t NumElements;

public:
    TDisjointIntervalTree()
        : NumElements()
    {
    }

    void Insert(const T t) {
        InsertInterval(t, t + 1);
    }

    // we assume that none of elements from [begin, end) belong to tree.
    void InsertInterval(const T begin, const T end) {
        InsertIntervalImpl(begin, end);
        NumElements += (size_t)(end - begin);
    }

    bool Has(const T t) const {
        return const_cast<TThis*>(this)->FindContaining(t) != Tree.end();
    }

    TConstIterator FindContaining(const T t) const {
        return const_cast<TThis*>(this)->FindContaining(t);
    }

    bool Erase(const T t) {
        TIterator n = FindContaining(t);
        if (n == Tree.end()) {
            return false;
        }

        --NumElements;

        T& begin = const_cast<T&>(n->first);
        T& end = const_cast<T&>(n->second);

        // optimization hack
        if (t == begin) {
            if (++begin == end) { // OK to change key since intervals do not intersect
                Tree.erase(n);
                return true;
            }

        } else if (t == end - 1) {
            --end;

        } else {
            const T e = end;
            end = t;
            InsertIntervalImpl(t + 1, e);
        }

        Y_ASSERT(begin < end);
        return true;
    }

    void Swap(TDisjointIntervalTree& rhv) {
        Tree.swap(rhv.Tree);
        std::swap(NumElements, rhv.NumElements);
    }

    void Clear() {
        Tree.clear();
        NumElements = 0;
    }

    bool Empty() const {
        return Tree.empty();
    }

    size_t GetNumElements() const {
        return NumElements;
    }

    size_t GetNumIntervals() const {
        return Tree.size();
    }

    T Min() const {
        Y_ASSERT(!Empty());
        return Tree.begin()->first;
    }

    T Max() const {
        Y_ASSERT(!Empty());
        return Tree.rbegin()->second;
    }

    TConstIterator begin() const {
        return Tree.begin();
    }

    TConstIterator end() const {
        return Tree.end();
    }

private:
    void InsertIntervalImpl(const T begin, const T end) {
        Y_ASSERT(begin < end);

        TIterator l = Tree.lower_bound(begin);
        TIterator p = Tree.end();
        if (l != Tree.begin()) {
            p = l;
            --p;
        }

#ifndef NDEBUG
        TIterator u = Tree.upper_bound(begin);
        Y_VERIFY_DEBUG(u == Tree.end() || u->first >= end, "Trying to add [%" PRIu64 ", %" PRIu64 ") which intersects with existing [%" PRIu64 ", %" PRIu64 ")", begin, end, u->first, u->second);
        Y_VERIFY_DEBUG(l == Tree.end() || l == u, "Trying to add [%" PRIu64 ", %" PRIu64 ") which intersects with existing [%" PRIu64 ", %" PRIu64 ")", begin, end, l->first, l->second);
        Y_VERIFY_DEBUG(p == Tree.end() || p->second <= begin, "Trying to add [%" PRIu64 ", %" PRIu64 ") which intersects with existing [%" PRIu64 ", %" PRIu64 ")", begin, end, p->first, p->second);
#endif

        // try to extend interval
        if (p != Tree.end() && p->second == begin) {
            p->second = end;
        } else {
            Tree.insert(std::make_pair(begin, end));
        }
    }

    TIterator FindContaining(const T t) {
        TIterator l = Tree.lower_bound(t);
        if (l != Tree.end()) {
            if (l->first == t) {
                return l;
            }
            Y_ASSERT(l->first > t);

            if (l == Tree.begin()) {
                return Tree.end();
            }

            --l;
            Y_ASSERT(l->first != t);

            if (l->first < t && t < l->second) {
                return l;
            }

        } else if (!Tree.empty()) { // l is larger than Begin of any interval, but maybe it belongs to last interval?
            TReverseIterator last = Tree.rbegin();
            Y_ASSERT(last->first != t);

            if (last->first < t && t < last->second) {
                return (++last).base();
            }
        }
        return Tree.end();
    }
};