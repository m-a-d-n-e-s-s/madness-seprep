/*
  This file is part of MADNESS.
  
  Copyright (C) 2007,2010 Oak Ridge National Laboratory
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
  
  For more information please contact:
  
  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367
  
  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680
  
  $Id$
*/

/**
  \file mra/sdf_shape_3D.h
  \brief Implements the SignedDFInterface for common 3-D geometric objects.
  \ingroup mrabcint

  This file provides signed distance functions for common 3-D geometric objects:
  - Plane
  - Sphere
  - Cone
  - Paraboloid
  - Box
  - Cube
  - Ellipsoid
  - Cylinder
  
  \note The signed distance functions should be the shortest distance between
  a point and \b any point on the surface.  This is hard to calculate in many
  cases, so we use contours here.  The surface layer may not be equally thick
  around all points on the surface.  Some surfaces (plane, sphere, paraboloid)
  use the exact signed distance functions.  All others use the contours, which
  may be extremely problematic and cause excessive refinement.  The sdf
  function of the sphere class outlines how to calculate the exact signed
  distance functions, if needed.
*/  
  
#ifndef MADNESS_MRA_SDF_SHAPE_3D_H__INCLUDED
#define MADNESS_MRA_SDF_SHAPE_3D_H__INCLUDED

#include <mra/sdf_domainmask.h>

namespace madness {

    /// \brief A plane surface (3 dimensions)
    class SDFPlane : public SignedDFInterface<3> {
    protected:
        const coord_3d normal; ///< The normal vector pointing OUTSIDE the surface
        const coord_3d point; ///< A point in the plane

    public:
        
        /** \brief SDF for a plane transecting the entire simulation volume

            \param normal The outward normal definining the plane
            \param point A point in the plane */
        SDFPlane(const coord_3d& normal, const coord_3d& point)
            : normal(normal*(1.0/sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2])))
            , point(point) 
        {}

        /** \brief Computes the normal distance

            This SDF is exact, and easy to show.

            \param pt Point at which to compute the distance from the surface
            \return The signed distance from the surface */
        double sdf(const coord_3d& pt) const {
            return (pt[0]-point[0])*normal[0] + (pt[1]-point[1])*normal[1] + (pt[2]-point[2])*normal[2];
        }

        /** \brief Computes the gradient of the SDF.

            \param pt Point at which to compute the gradient
            \return the gradient */
        coord_3d grad_sdf(const coord_3d& pt) const {
            MADNESS_EXCEPTION("gradient method is not yet implemented for this shape",0);
        }
    };

    /// \brief A spherical surface (3 dimensions)
    class SDFSphere : public SignedDFInterface<3> {
    protected:
        const double radius; ///< Radius of sphere
        const coord_3d center; ///< Center of sphere

    public:
        /** \brief SDF for a sphere

            \param radius The radius of the sphere
            \param center The center of the sphere */
        SDFSphere(const double radius, const coord_3d &center) 
            : radius(radius)
            , center(center) 
        {}

        /** \brief Computes the normal distance

            This SDF is exact, and easy to show.

            \param pt Point at which to compute the distance from the surface
            \return The signed distance from the surface */
        double sdf(const coord_3d& pt) const {
            double temp, r;
            int i;
            
            r = 0.0;
            for(i = 0; i < 3; ++i) {
                temp = pt[i] - center[i];
                r += temp * temp;
            }
            
            return sqrt(r) - radius;
        }

        /** \brief Computes the gradient of the SDF.

            \param pt Point at which to compute the gradient
            \return the gradient */
        coord_3d grad_sdf(const coord_3d& pt) const {
            double x = pt[0] - center[0];
            double y = pt[1] - center[1];
            double z = pt[2] - center[2];
            double r = sqrt(x*x + y*y + z*z);
            coord_3d g;
            g[0] = x/r;
            g[1] = y/r;
            g[2] = z/r;
            return g;
        }
    };

    /** \brief A cone (3 dimensions)

        The cone is defined by 

        \f[ \sqrt{x^2 + y^2} - c * z = 0 \f]

        where \f$ z \f$ is along the cone's axis. */
    class SDFCone : public SignedDFInterface<3> {
    protected:
        const coord_3d apex; ///< The apex
        const double c; ///< The radius
        const coord_3d dir; ///< The direction of the axis, from the apex INSIDE

    public:
        /** \brief Constructor for cone

            \param c Parameter \f$ c \f$ in the definition of the cone
            \param apex Apex of cone
            \param direc Oriented axis of the cone */
        SDFCone(const double c, const coord_3d &apex, const coord_3d &direc) 
            : apex(apex)
            , c(c)
            , dir(direc*(1.0/sqrt(direc[0]*direc[0] + direc[1]*direc[1] + direc[2]*direc[2])))
        {}

        /** \brief Computes the normal distance

            This SDF naively uses contours, and should be improved before
            serious usage.

            \param pt Point at which to compute the distance from the surface
            \return The signed distance from the surface */
        double sdf(const coord_3d& pt) const {
            coord_3d diff;
            unsigned int i;
            double dotp;
            
            for(i = 0; i < 3; ++i)
                diff[i] = pt[i] - apex[i];
            
            dotp = diff[0]*dir[0] + diff[1]*dir[1] + diff[2]*dir[2];
            
            for(i = 0; i < 3; ++i)
                diff[i] -= dotp * dir[i];
            
            return sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2])
                - c * dotp;
        }

        /** \brief Computes the gradient of the SDF.

            \param pt Point at which to compute the gradient
            \return the gradient */
        coord_3d grad_sdf(const coord_3d& pt) const {
            MADNESS_EXCEPTION("gradient method is not yet implemented for this shape",0);
        }
    };

    /** \brief A paraboloid (3 dimensions)

        The surface is defined by 

        \f[ x^2 + y^2 - c * z == 0 \f]

        where \f$ z \f$ is along the paraboloid's axis. */
    class SDFParaboloid : public SignedDFInterface<3> {
    protected:
        const coord_3d apex; ///< The apex
        const double c; ///< Curvature/radius of the surface
        const coord_3d dir;///< The direction of the axis, from the apex INSIDE

    public:
        /** \brief Constructor for paraboloid

            \param c Parameter \f$ c \f$ in the definition of the paraboloid
            \param apex Apex of paraboloid
            \param direc Oriented axis of the paraboloid */
        SDFParaboloid(const double c, const coord_3d &apex, const coord_3d &direc) 
            : apex(apex)
            , c(c)
            , dir(direc*(1.0/sqrt(direc[0]*direc[0] + direc[1]*direc[1] + direc[2]*direc[2])))
        {}

        /** \brief Computes the normal distance.

            This SDF is exact.

            Given a point, pt=(x, y, z), the goal is to find another point,
            pt0=(x0, y0, z0), on the surface that minimizes |pt - pt0|^2.  The
            root of this minimized square distance (and a sign) is the sdf.

            For simplicity (here), I will assume that the paraboloid's axis
            is along the positive z-axis and that the origin is the apex.
            Note that the code does NOT make these assumptions.

            Thus, we want to minimize
               (x-x0)^2 + (y-y0)^2 + (z-z0)^2
            subject to
               x0^2 + y0^2 - c z0 == 0.

            Using Lagrange multipliers, the system of equations is
            -2(x-x0) == L 2 x0
            -2(y-y0) == L 2 y0
            -2(z-z0) == L (-c)
            x0^2 + y0^2 - c z0 == 0.

            After some algebra, we get a cubic equation for L,
            (x^2 + y^2 - c z) + (2c z + c^2/2) L - c (z + c) L^2
               + c^2/2 L^3 == 0

            This can be solved analytically in Mathematica, producing a long,
            messy equation that is implemented below.  There are three complex
            solutions to this equation; at least one is always real.  For
            later, we choose the Lagrange multiplier that is real and smallest
            in magnitude.

            Note that solution to this cubic equation uses both a square root
            and a cubic root; we track all six combinations even though there
            are only three distinct answers.  (Perhaps someone will simplify
            the code someday).

            Once we have the correct Lagrange multiplier,
            |pt - pt0|^2 = c L^2 (z - L c/2 + c/4).
            The square root of this quantity (with the appropriate sign for
            inside/outside) gives the sdf.
            
            \param pt Point at which to compute the distance from the surface
            \return The signed distance from the surface */
        double sdf(const coord_3d& pt) const {
            coord_3d diff;
            unsigned int i, n;
            double dotp, d;
            std::complex<double> root[6];
            std::complex<double> cubicroot(-0.5, sqrt(3.0)*0.5);
            double lambda;

            for(i = 0; i < 3; ++i)
                diff[i] = pt[i] - apex[i];
            
            dotp = diff[0]*dir[0] + diff[1]*dir[1] + diff[2]*dir[2];
            
            for(i = 0; i < 3; ++i)
                diff[i] -= dotp * dir[i];

            d = diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]
                - c * dotp;

            root[0] = 27.0*c*(d + c*dotp)*(2.0*c*c*c + 15.0*c*c*dotp
                - 16.0*dotp*dotp*dotp + 24.0*c*dotp*dotp
                + 27.0*c*d);

            // take the square root and use both signs
            root[0] = sqrt(root[0]);
            root[1] = -root[0];

            lambda = -c*c*c - 21.0*c*c*dotp + 8.0*dotp*dotp*dotp
                - 27.0*c*d - 12.0*c*dotp*dotp;

            for(i = 0; i < 2; ++i) {
                root[i] += lambda;

                // take the cubic root and account for complex roots
                root[i] = pow(root[i], 1.0/3.0);
            }

            for(i = 2; i < 6; ++i)
                root[i] = root[i-2] * cubicroot;

            // finalize the calculation
            // and get the smallest, real root
            n = 0; // the number of real roots thus encountered
            for(i = 0; i < 6; ++i) {
                root[i] = (2.0*(c+dotp) + root[i] + (c-2.0*dotp)*(c-2.0*dotp)
                    / root[i]) / (3.0*c);

                // is this root real?
                if(fabs(imag(root[i])) < 1.0e-12) {
                    if(n == 0) {
                        lambda = real(root[i]);
                    }
                    else {
                        if(real(root[i]) < lambda)
                            lambda = real(root[i]);
                    }
                    ++n;
                }
            }
            MADNESS_ASSERT(n > 0); // we should have at least one real root...

            // now that we have the Lagrange multiplier, get the distance
            lambda = c*lambda*lambda*(dotp - lambda*c*0.5 + c*0.25);
            MADNESS_ASSERT(lambda >= 0.0); // this shouldn't be negative
            lambda = sqrt(lambda);

            if(d > 0.0)
                return lambda;
            else
                return -lambda;
        }

        /** \brief Computes the gradient of the SDF.

            \param pt Point at which to compute the gradient
            \return the gradient */
        coord_3d grad_sdf(const coord_3d& pt) const {
            MADNESS_EXCEPTION("gradient method is not yet implemented for this shape",0);
        }
    };

    /** \brief A box (3 dimensions)

        \note LIMIT -- the 3 primary axes must be x, y, and z */
    class SDFBox : public SignedDFInterface<3> {
    protected:
        const coord_3d lengths;  ///< Half the length of each side of the box
        const coord_3d center; ///< the center of the box

    public:
        /** \brief Constructor for box

            \param length The lengths of the box
            \param center The center of the box */
        SDFBox(const coord_3d& length, const coord_3d& center) 
            : lengths(length*0.5), center(center) 
        {}

        /** \brief Computes the normal distance

            This SDF naively uses contours, and should be improved before
            serious usage.  If far from the corners, the SDF is easy (similar
            to a plane), and is essentially what's implemented.

            \param pt Point at which to compute the distance from the surface
            \return The signed distance from the surface */
        double sdf(const coord_3d& pt) const {
            double diff, max;
            int i;
            
            max = fabs(pt[0] - center[0]) - lengths[0];
            for(i = 1; i < 3; ++i) {
                diff = fabs(pt[i] - center[i]) - lengths[i];
                if(diff > max)
                    max = diff;
            }
            
            return max;
        }

        /** Computes the gradient of the SDF.

            \param pt Point at which to compute the gradient
            \return the gradient */
        coord_3d grad_sdf(const coord_3d& pt) const {
            MADNESS_EXCEPTION("gradient method is not yet implemented for this shape",0);
        }
    };

    /** \brief A cube (3 dimensions)

        \note LIMIT -- the 3 primary axes must be x, y, and z */
    class SDFCube : public SDFBox {
    public:
        /** \brief Constructor for box

            \param length The length of each side of the cube
            \param center The center of the cube */
        SDFCube(const double length, const coord_3d& center) 
            : SDFBox(coord_3d(length), center)
        {}
    };

    /** \brief An ellipsoid (3 dimensions)
    
        \note LIMIT -- the 3 primary axes must be x, y, and z */
    class SDFEllipsoid : public SignedDFInterface<3> {
    protected:
        coord_3d radii; ///< the directional radii
        coord_3d center; ///< the center
        
    public:
        /** \brief Constructor for ellipsoid

            \param radii The directional radii of the ellipsoid
            \param center The center of the ellipsoid */
        SDFEllipsoid(const coord_3d& radii, const coord_3d& center) 
            : radii(radii)
            , center(center) 
        {}
        
        /** \brief Computes the normal distance

            This SDF naively uses contours, and should be improved before
            serious usage.

            \param pt Point at which to compute the distance from the surface
            \return The signed distance from the surface */
        double sdf(const coord_3d& pt) const {
            double quot, sum;
            int i;
            
            sum = 0.0;
            for(i = 0; i < 3; ++i) {
                quot = (pt[i] - center[i]) / radii[i];
                sum += quot * quot;
            }
            
            return sum - 1.0;
        }

        /** Computes the gradient of the SDF.

            \param pt Point at which to compute the gradient
            \return the gradient */
        coord_3d grad_sdf(const coord_3d& pt) const {
            MADNESS_EXCEPTION("gradient method is not yet implemented for this shape",0);
        }
    };
    
    /// \brief A cylinder (3 dimensions)
    class SDFCylinder : public SignedDFInterface<3> {
    protected:
        double radius; ///< the radius of the cylinder
        double a; ///< half the length of the cylinder
        coord_3d center; ///< the central axial point of the cylinder (distance a from both ends)
        coord_3d axis; ///< the axial direction of the cylinder

    public:
        /** \brief Constructor for cylinder

            \param radius The radius of the cylinder
            \param length The length of the cylinder
            \param axpt The central axial point of the cylinder (equidistant
                        from both ends)
            \param axis The axial direction of the cylinder */
        SDFCylinder(const double radius, const double length, const coord_3d& axpt, const coord_3d& axis) 
            : radius(radius)
            , a(length / 2.0)
            , center(axpt)
            , axis(axis*(1.0/sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]))) 
        {}
        
        /** \brief Computes the normal distance

            This SDF naively uses contours, and should be improved before
            serious usage.

            \param pt Point at which to compute the distance from the surface
            \return The signed distance from the surface */
        double sdf(const coord_3d& pt) const {
            double dist;
            coord_3d rel, radial;
            int i;
            
            // axial distance
            dist = 0.0;
            for(i = 0; i < 3; ++i) {
                rel[i] = pt[i] - center[i];
                dist += rel[i] * axis[i];
            }
            
            // get the radial component
            for(i = 0; i < 3; ++i)
                radial[i] = rel[i] - dist * axis[i];
            
            return std::max(fabs(dist) - a, sqrt(radial[0]*radial[0] + radial[1]*radial[1]
                                                 + radial[2]*radial[2]) - radius);
        }

        /** Computes the gradient of the SDF.

            \param pt Point at which to compute the gradient
            \return the gradient */
        coord_3d grad_sdf(const coord_3d& pt) const {
            MADNESS_EXCEPTION("gradient method is not yet implemented for this shape",0);
        }
    };

} // end of madness namespace

#endif // MADNESS_MRA_SDF_SHAPE_3D_H__INCLUDED
