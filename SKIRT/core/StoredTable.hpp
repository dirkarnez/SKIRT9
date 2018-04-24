/*//////////////////////////////////////////////////////////////////
////     The SKIRT project -- advanced radiative transfer       ////
////       © Astronomical Observatory, Ghent University         ////
///////////////////////////////////////////////////////////////// */

#ifndef STOREDTABLE_HPP
#define STOREDTABLE_HPP

#include "Array.hpp"
#include "CompileTimeUtils.hpp"
#include "NR.hpp"
#include "StoredTableImpl.hpp"
#include <array>

////////////////////////////////////////////////////////////////////

/** An instance of the StoredTable<N> class template provides access to the contents of a
    particular resource file in the SKIRT stored table format (i.e. a "stored table").

    Stored table file format
    ------------------------

    A stored table includes the names of the quantities on the axes (e.g. wavelength and grain
    size) and those being tabulated (e.g. absorption and scattering efficiencies), in addition to
    the grid points for each axis and the tabulated data values. All grid points and values are
    stored as binary data in the form of 64-bit floating-point numbers, and are always given in SI
    units. The format is designed so that it is easy to calculate the offset, relative to the start
    of the file, to any particular data value. More specifically, a stored table file is
    essentially a sequence of 8-byte data items. A data item can have one of three types:
        - string: 1 to 8 printable and non-whitespace 7-bit ASCII characters, padded with spaces
          to fill 8 bytes if needed;
        - unsigned integer: 64-bit integer in little-endian byte order;
        - floating point: 64-bit double (IEEE 754) in little-endian byte order.

    The overall layout is as follows:
        - SKIRT name/version tag
        - Endianness tag
        - numAxes
        - axisName (x numAxes)
        - axisUnit (x numAxes)
        - axisScale (x numAxes)
        - [ numPoints  axisPoint (x numPoints) ] (x numAxes)
        - numQuantities
        - quantityName (x numQuantities)
        - quantityUnit (x numQuantities)
        - quantityScale (x numQuantities)
        - value (x numQuantities x numPoints1 x ... x numPointsN)
        - end-of-file tag

    The values are ordered so that the quantity values for a particular point are next to each
    other, the first axis index varies most rapidly, and the last axis index varies least rapidly.

    The StoredTable<N> class template
    ---------------------------------

    The StoredTable<N> template parameter \em N specifies the number of axes in the stored table,
    and thus the number of axis values needed to retrieve a tabulated quantity from the table. Each
    StoredTable<N> instance represents a single tabulated quantity. Accessing the full contents of
    a stored table resource file with multiple tabulated quantities requires a seperate
    StoredTable<N> instance for each of those quantities.

    The default constructor creates an invalid stored table instance. The alternate constructor and
    the open() function associate a particular stored table resource file with the stored table
    instance. The number of axes in this stored table resource file must match the template
    parameter \em N. Also, the axis names and the corresponding units in the file, and one of the
    tabulated quantity names and its corresponding unit in the file, must match the information
    passed to the alternate constructor or the open() function. The destructor automatically
    releases the file association and any related resources.

    The parenthesis operator returns the quantity represented by the StoredTable<N> for the \em N
    specified axis values, interpolated from the tabulated values. Other functions offer access
    for specific purposes, such as constructing a cumulative distribution function along one axis,
    given values for the other axes.

    Implementation and performance
    ------------------------------

    A StoredTable<N> instance acquires a read-only memory map on the associated stored table
    resource file as opposed to actually reading the file contents into memory through regular file
    I/O operations. This has some important, mostly positive, consequences.

    Acquiring a memory map establishes a mapping between "pages" of system-defined size in the
    logical address space of a process and the contents of the "backing file", in this case the
    stored table resource file. This operation is simple and thus very fast. From then on, the
    operating system automatically loads pages from the backing file into physical memory as they
    become needed because the program addresses an item in the logical memory range of the page.
    Conversely, the operating system automatically removes pages from physical memory if available
    memory becomes tight. In effect, the operating system automatically manages a high-performance
    caching mechanism on stored tables.

    Three important use cases benefit greatly from this mechanism. Firstly, a large resource file
    can be left associated with a StoredTable<N> instance for the duration of the program, even if
    it is used only sporadically. When memory is tight, infrequently used portions of the data will
    automatically be removed from memory and reloaded later if needed. Secondly, there is little
    overhead in constructing a StoredTable<N> instance (and possibly destroying it shortly
    thereafter) even when the program needs only a small portion of the file contents. And thirdly,
    because all StoredTable<N> instances associated with a given stored table resource file share
    the same memory map on that file, using a seperate instance for each quantity in the stored
    table incurs very little overhead.

    Moreover, most operating systems share memory maps between processes. For a parallel program
    using MPI, this means that all processes running on the same compute node share a single memory
    copy of the resources they employ. Also, most operating systems keep the memory map caches
    alive between consecutive invocations of a program (assuming memory is available), increasing
    performance when, for example, interactively testing the program.

    On the downside, a program requesting a huge chunk of data from a large stored table in a
    serial fashion would run faster using regular file I/O, because the separate page loads take
    more time than sequentially reading data in bulk. More importantly, performance usually
    degrades rapidly (to the point where the program no longer performs any useful work) when the
    system is forced to constantly remove and reload pages because there is not enough memory to
    hold the data needed for a particular phase in the program. And finally, the run-time
    performance of a program becomes somewhat unpredicable because the speed of accessing resources
    depends heavily on the previous state of the operating system caches.
*/
template<size_t N> class StoredTable
{
    static_assert(N >= 1, "StoredTable number of axes must be at least 1");

    // ================== Constructing ==================

public:
    /** The default constructor constructs an invalid stored table instance. The user must call the
        open() function to associate the stored table instance with a particular stored table
        resource file. Calling any of the other functions before calling open() results in
        undefined behavior (usually a crash). */
    StoredTable() { }

    /** This alternate constructor constructs a stored table instance and immediately associates a
        given stored table resource file with it by calling the open() function. Refer to the
        open() function for a description of the arguments and of its operation. */
    StoredTable(const SimulationItem* item, string filename, string axes, string quantity)
    {
        open(item, filename, axes, quantity);
    }

    /** This function associates a given stored table resource file with the stored table instance.
        If such an association already exists, this function throws a fatal error. Conversely,
        calling any of the other functions before an association exists results in undefined
        behavior (usually a crash).

        The \em item argument specifies a simulation item in the hierarchy of the caller (usually
        the caller itself) used to retrieve an appropriate logger.

        The \em filename argument specifies the filename of the resource, without any directory
        segments. The resource file must have the ".stab" filename extension, which will be added
        to the specified filename if needed.

        First of all, the number of axes in this stored table resource file must match the template
        parameter \em N. Furthermore, the axes names in the resource file and the corresponding
        units for each axis must match the information specified in the \em axes argument. Finally,
        one of the tabulated quantity names in the resource file and its corresponding unit must
        match the information specified in the \em quantity argument. For a stored table resource
        file with multiple tabulated quantities, the \em quantity argument at the same time
        determines which of these quantities will be associated with the stored table instance.

        The string passed to the \em axes argument must have the syntax
        "name1(unit1),...,nameN(unitN)". In other words, a unit string between parenthesis follows
        each axis name, and the specifications for different axes are separated by a comma. For
        example, "lambda(m),a(m)". Whitespace is not allowed. The string passed to the \em quantity
        argument must have a similar syntax, for a single name/unit combination. Examples include
        "Llambda(W/m)", "Qabs(1)", and "h(J/m3)".

        In summary, this function (1) locates the specified stored table resource file, (2)
        acquires a memory map on the file, (3) verifies that the stored table matches all
        requirements, and (4) stores relevant information in data members. If any of these steps
        fail, the function throws a fatal error. */
    void open(const SimulationItem* item, string filename, string axes, string quantity)
    {
        StoredTable_Impl::open(N, item, filename, axes, quantity,
                               _filePath, _axBeg.begin(), &_qtyBeg, _axLen.begin(), &_qtyStep,
                               _axLog.begin(), &_qtyLog);
    }

    /** The destructor breaks the association with a stored table resource file established by the
        alternate constructor or the open() function, if there is any. In practice, this simply
        means releasing the memory map on the associated file. */
    ~StoredTable() { StoredTable_Impl::close(_filePath); }

    // ================== Accessing values ==================

public:

    /** This function returns the value of the quantity represented by this stored table for the
        specified axes values, interpolated over the grid points of the actual tabulated values in
        all dimensions. The function uses linear or logarithmic interpolation for the axes and
        quantity values according to the flags specified in the stored table. Out-of-range axes
        values are automatically clamped to the corresponding outer grid point. */
    template <typename... Values, typename = std::enable_if_t<CompileTimeUtils::isFloatArgList<N, Values...>()>>
    double operator()(Values... values) const
    {
        // storage for each axis
        std::array<double, N> value = {{ static_cast<double>(values)... }};
        std::array<size_t, N> i2;  // upper grid bin boundary index
        std::array<double, N> f;  // fraction of axis value in bin

        // precompute for each axis
        for (size_t k = 0; k!=N; ++k)
        {
            // get the index of the upper border of the axis grid bin containing the specified axis value
            double x = value[k];
            size_t right = std::lower_bound(_axBeg[k], _axBeg[k]+_axLen[k], x) - _axBeg[k];

            // if the value is beyond the grid borders, adjust both the bin border and the value
            if (right == 0)
            {
                right++;
                x = _axBeg[k][0];
            }
            else if (right == _axLen[k])
            {
                right--;
                x = _axBeg[k][right];
            }
            i2[k] = right;

            // get the axis values at the grid borders
            double x1 = _axBeg[k][right-1];
            double x2 = _axBeg[k][right];

            // if requested, compute logarithm of coordinate values
            if (_axLog[k])
            {
                x = log10(x);
                x1 = log10(x1);
                x2 = log10(x2);
            }

            // calculate the fraction of the requested axis value in the bin
            f[k] = (x-x1)/(x2-x1);
        }

        // storage for each term in the interpolation
        constexpr size_t numTerms = 1 << N;    // there are 2^N terms
        std::array<double, numTerms> ff;       // front factor
        std::array<double, numTerms> yy;       // tabulated value

        // perform logarithmic interpolation of y value if requested and all bordering values are positive
        bool logy = _qtyLog;

        // determine front factor and tabulated value for each term of the interplation
        std::array<size_t, N> indices;         // storage for indices of the current term
        for (size_t t = 0; t!=numTerms; ++t)
        {
            // use the binary representation of the term index to determine left/right for each axis
            size_t term = t;    // temporary version of term index that will be bit-shifted
            double front = 1.;
            for (size_t k = 0; k!=N; ++k)
            {
                size_t left = term & 1;    // lowest significant digit = 1 means lower border
                indices[k] = i2[k] - left;
                front *= left ? (1-f[k]) : f[k];
                term >>= 1;
            }
            ff[t] = front;
            yy[t] = valueAtIndices(indices);
            if (yy[t]<=0) logy = false;
        }

        // calculate sum
        double y = 0.;
        if (logy)
        {
            for (size_t t = 0; t!=numTerms; ++t) y += ff[t] * log10(yy[t]);
            y = pow(10,y);
        }
        else
        {
            for (size_t t = 0; t!=numTerms; ++t) y += ff[t] * yy[t];
        }
        return y;
    }

    /** For a one-dimensional table only, this function returns the value of the quantity
        represented by the stored table for the specified axes value, interpolated over the grid
        points of the actual tabulated values. The function uses linear or logarithmic
        interpolation for the axis and quantity values according to the flags specified in the
        stored table. Out-of-range axes values are automatically clamped to the corresponding outer
        grid point. */
    template <typename Value, typename = std::enable_if_t<N==1 && CompileTimeUtils::isFloatArgList<1, Value>()>>
    double operator[](Value value) const
    {
        return operator()(value);
    }

    // ------------------------------------------

    /** This function constructs the normalized cumulative distribution function for the tabulated
        quantity across a given range in the first axis (the \em xmin and \em xmax arguments),
        using given fixed values for the other axes, if any (the arguments at the end of the list).
        If the internal representation of the table includes at least the specified number of
        minimum bins (the \em minBins argument) in the specified range of the first axis, then the
        internal grid points are used, because they should offer optimal resolution everywhere.
        Otherwise, a new grid is constructed (linear or logarithmic depending on the internal
        representation of the first axis) with the specified number of minimum bins. This can be
        useful to interpolate on a finer grid than the internal grid of the table.

        The resulting first-axis grid is constructed into \em xv, and the corresponding normalized
        cumulative distribution is constructed into \em Yv. Assuming that the function decided to
        return $n$ bins, these two arrays will each have $n+1$ elements (border points). In all
        cases, xv[0]=xmin, xv[n]=xmax, Yv[0]=0, and Yv[n]=1. The function returns the normalization
        factor, i.e. the value of Yv[n] before normalization.

        If any of the axes values, including the \em xmin or \em xmax specifying the range for the
        first axis, are out of range of the internal grid, extra quantity values are fabricated by
        clamping the interpolation to the corresponding outer grid point. */
    template <typename... Values, typename = std::enable_if_t<CompileTimeUtils::isFloatArgList<N-1, Values...>()>>
    double cdf(Array& xv, Array& Yv, size_t minBins, double xmin, double xmax, Values... values) const
    {
        // there must be at least one bin  (n = number of bins; n+1 = number of border points)
        size_t n = std::max(static_cast<size_t>(1), minBins);

        // if the number of grid points is sufficient, copy the relevant portion of the internal axis grid
        size_t minRight = std::lower_bound(_axBeg[0], _axBeg[0]+_axLen[0], xmin) - _axBeg[0];
        size_t maxRight = std::lower_bound(_axBeg[0], _axBeg[0]+_axLen[0], xmax) - _axBeg[0];
        if (minRight + n <= maxRight  )
        {
            n = 1 + maxRight - minRight;  // n = number of bins
            xv.resize(n+1);               // n+1 = number of border points
            size_t i = 0;                 // i = index in target array
            xv[i++] = xmin;               // j = index in internal grid array
            for (size_t j = minRight; j < maxRight; ) xv[i++] = _axBeg[0][j++];
            xv[i++] = xmax;
        }
        // otherwise, build a new grid with the requested number of bins
        else
        {
            if (_axLog[0]) NR::buildLogGrid(xv, xmin, xmax, n);
            else NR::buildLinearGrid(xv, xmin, xmax, n);
        }

        // resize Y array; also sets Yv[0] to zero
        Yv.resize(n+1);

        // calculate cumulative values corresponding to each x grid point (and any extra axis values)
        for (size_t i = 0; i!=n; ++i)
        {
            double dx = xv[i+1] - xv[i];
            double y = operator()(xv[i], values...);
            Yv[i+1] = Yv[i] + y*dx;
        }

        // normalize cumulative distribution and return normalization factor
        double norm = Yv[n];
        Yv /= norm;
        return norm;
    }

    // ================== Accessing the raw data ==================

private:
    /** This template function returns a copy of the value at the specified N indices. There is no
        range checking. Out-of-range index values cause unpredictable behavior. */
    template <typename... Indices, typename = std::enable_if_t<CompileTimeUtils::isFloatArgList<N, Indices...>()>>
    double valueAtIndices(Indices... indices) const
    {
        return _qtyBeg[flattenedIndex(std::array<size_t, N>({{ static_cast<size_t>(indices)... }}))];
    }

    /** This function returns a copy of the value at the specified N indices. There is no range
        checking. Out-of-range index values cause unpredictable behavior. */
    double valueAtIndices(const std::array<size_t, N>& indices) const
    {
        return _qtyBeg[flattenedIndex(indices)];
    }

    /** This function returns the flattened index in the underlying data array for the specified N
        indices. */
    size_t flattenedIndex(const std::array<size_t, N>& indices) const
    {
        size_t result = indices[N-1];
        for (size_t k = N-2; k<N; --k) result = result * _axLen[k] + indices[k];
        return result * _qtyStep;
    }

    // ================== Data members ==================

private:
    string _filePath;                       // the canonical path to the associated stored table file
    std::array<const double*, N> _axBeg;    // pointer to first grid point for each axis
    const double* _qtyBeg;                  // pointer to first quantity value
    std::array<size_t, N> _axLen;           // number of grid points for each axis
    size_t _qtyStep;                        // step size from one quantity value to the next (1=adjacent)
    std::array<bool, N> _axLog;             // interpolation type (true=log, false=linear) for each axis
    bool _qtyLog;                           // interpolation type (true=log, false=linear) for quantity
};

////////////////////////////////////////////////////////////////////

#endif
