/*//////////////////////////////////////////////////////////////////
////     The SKIRT project -- advanced radiative transfer       ////
////       © Astronomical Observatory, Ghent University         ////
///////////////////////////////////////////////////////////////// */

#ifndef DISJOINTWAVELENGTHGRID_HPP
#define DISJOINTWAVELENGTHGRID_HPP

#include "WavelengthGrid.hpp"

//////////////////////////////////////////////////////////////////////

/** DisjointWavelengthGrid is an abstract class that represents wavelength grids with
    straightforward, non-overlapping bins with constant transmission across each bin.

    Specifically, a disjoint wavelength grid consists of \f$N>0\f$ non-overlapping but possibly
    adjacent wavelength bins in increasing wavelength order. Each bin is defined by its left and
    right borders. The left border is considered to be inside of the bin; the right border is
    considered to be outside of the bin. Furthermore, each bin is characterized by a characteristic
    wavelength that falls inside the bin, i.e. \f$\lambda^\mathrm{left}_\ell \le
    \lambda^\mathrm{c}_\ell < \lambda^\mathrm{right}_\ell, \ell=0\dots N-1\f$. Finally, each bin of
    course has an associated bin width, \f$\lambda^\mathrm{right}_\ell -
    \lambda^\mathrm{left}_\ell, \ell=0\dots N-1\f$.

    A DisjointWavelengthGrid subclass is expected to invoke one of the setWavelengthXXX() functions
    during setup to intialize the wavelength grid. The current implementation offers two such
    functions: one to specify a consecutive range of adjacent wavelength bins given a list of
    characteric wavelengths, and another one to specify distinct, nonadjacent wavelength bins given
    a list of characteric wavelengths and a relative bin width. Other options can be added as the
    need arises. */
class DisjointWavelengthGrid : public WavelengthGrid
{
    ITEM_ABSTRACT(DisjointWavelengthGrid, WavelengthGrid, "a wavelength grid with non-overlapping bins")
    ITEM_END()

    //============= Construction - Setup - Destruction =============

protected:
    /** This function verifies that the wavelength bins have been initialized by a subclass calling
        the one of the setWavelengthXXX() functions of this class in their setupSelfBefore()
        function. */
    void setupSelfAfter() override;

    /** This function initializes the wavelength grid to a consecutive range of adjacent wavelength
        bins given a list of characteric wavelengths. This function or one of its alternatives
        should be called from the setupSelfBefore() function in each WavelengthGrid subclass. The
        subclass determines a list of characteric wavelengths according to some predefined scheme,
        and the bin borders and bin widths are automatically determined from that list by this
        function. If the specified wavelength list is empty, or if there are duplicate values
        (which would lead to empty bins), the function throws a fatal error.

        Specifically, the function first sorts the specified characteristic wavelengths in
        ascending order and then calculates the bin borders as follows. The inner border between
        two consecutive characteristic wavelengths is placed at the geometric mean of those two
        wavelengths, i.e. \f$\lambda^\mathrm{right}_{\ell-1} = \lambda^\mathrm{left}_\ell =
        \sqrt{\lambda^\mathrm{c}_{\ell-1}\lambda^\mathrm{c}_\ell}\;, \ell=1\dots N-1\f$. The outer
        borders at the very left and right of the wavelength range are placed just outside of the
        range, i.e. \f$\lambda^\mathrm{left}_0 = \lambda^\mathrm{c}_0(1-1/1000)\f$ and
        \f$\lambda^\mathrm{right}_{N-1} = \lambda^\mathrm{c}_{N-1}(1+1/1000)\f$. Finally the
        function trivially calculates the wavelength bin widths from the bin borders. */
    void setWavelengthRange(const Array& lambdav);

    /** This function initializes the wavelength grid to a set of distinct, nonadjacent wavelength
        bins given a list of characteric wavelengths and a relative half bin width. This function
        or one of its alternatives should be called from the setupSelfBefore() function in each
        WavelengthGrid subclass. The subclass determines a list of characteric wavelengths and a
        relative half bin width, and the bin borders and bin widths are automatically calculated
        from that information by this function. If the specified wavelength list is empty, or if
        the relative half bin width is not positive, or if the calculated bins overlap, the
        function throws a fatal error.

        Specifically, the function first sorts the specified characteristic wavelengths in
        ascending order and then calculates the bin borders using \f$\lambda^\mathrm{left}_\ell =
        \lambda^\mathrm{c}_\ell(1-w)\f$ and \f$\lambda^\mathrm{right}_\ell =
        \lambda^\mathrm{c}_\ell(1+w)\;, \ell=0\dots N-1\f$, where \f$w\f$ is the specified relative
        half bin width. If the \em constantWidth flag is true, the width for the shortest
        wavelength is used for all bin widths instead. Finally the function trivially calculates
        the wavelength bin widths from the bin borders. */
    void setWavelengthBins(const Array& lambdav, double relativeHalfWidth, bool constantWidth = false);

    //======================== Other Functions =======================

public:
    /** This function returns the number of bins, \f$N\f$, in the grid (or equivalently, the number
        of characteristic wavelengths). */
    int numBins() const override;

   /** This function returns the characteristic wavelength \f$\lambda^\mathrm{c}_\ell\f$
       corresponding to the index \f$\ell\f$. */
    double wavelength(int ell) const override;

    /** This function returns the left border of the wavelength bin corresponding to the index
        \f$\ell\f$, i.e. \f$\lambda^\mathrm{left}_\ell\f$. */
    double leftBorder(int ell) const override;

    /** This function returns the right border of the wavelength bin corresponding to the index
        \f$\ell\f$, i.e. \f$\lambda^\mathrm{right}_\ell\f$. */
    double rightBorder(int ell) const override;

    /** This function returns the width of the wavelength bin corresponding to the index
        \f$\ell\f$, i.e. \f$\lambda^\mathrm{right}_\ell - \lambda^\mathrm{left}_\ell\f$. */
    double effectiveWidth(int ell) const override;

    /** This function returns the relative transmission for the wavelength bin corresponding to the
        index \f$\ell\f$ at the wavelength \f$\lambda\f$. For the present class, it always returns
        1. */
    double transmission(int ell, double lambda) const override;

    /** This function returns a single-element list with the index \f$\ell\f$ of the wavelength bin
        that contains the specified wavelength \f$\lambda\f$, i.e. for which
        \f$\lambda^\mathrm{left}_\ell <= \lambda < \lambda^\mathrm{right}_\ell\f$. If \f$\lambda\f$
        does not lie inside one of the wavelength bins, the function returns an empty list. */
    vector<int> bins(double lambda) const override;

    /** This function returns the index \f$\ell\f$ of the wavelength bin that contains the
        specified wavelength \f$\lambda\f$, i.e. for which \f$\lambda^\mathrm{left}_\ell <= \lambda
        < \lambda^\mathrm{right}_\ell\f$. If \f$\lambda\f$ does not lie inside one of the
        wavelength bins, the function returns -1. */
    int bin(double lambda) const override;

    //======================== Data Members ========================

private:
    // subclasses should call setWavelengthXXX() in their setupSelfBefore() to initialize these tables
    Array _lambdav;      // N characteristic wavelengths
    Array _dlambdav;     // N wavelength bin widths
    Array _lambdaleftv;  // N left wavelength bin borders
    Array _lambdarightv; // N right wavelength bin widths
    Array _borderv;      // K=N+1 or K=N*2 ordered border points (depending on whether bins are adjacent)
    vector<int> _ellv;   // K+1 indices of the wavelength bins defined by the border points, or -1 if out of range
};

//////////////////////////////////////////////////////////////////////

#endif
