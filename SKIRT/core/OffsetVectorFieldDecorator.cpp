/*//////////////////////////////////////////////////////////////////
////     The SKIRT project -- advanced radiative transfer       ////
////       © Astronomical Observatory, Ghent University         ////
///////////////////////////////////////////////////////////////// */

#include "OffsetVectorFieldDecorator.hpp"

////////////////////////////////////////////////////////////////////

int OffsetVectorFieldDecorator::dimension() const
{
    return (_offsetX || _offsetY || _vectorField->dimension() == 3) ? 3 : 2;
}

////////////////////////////////////////////////////////////////////

Vec OffsetVectorFieldDecorator::vector(Position bfr) const
{
    double x, y, z;
    bfr.cartesian(x, y, z);
    return _vectorField->vector(Position(x - _offsetX, y - _offsetY, z - _offsetZ));
}

////////////////////////////////////////////////////////////////////