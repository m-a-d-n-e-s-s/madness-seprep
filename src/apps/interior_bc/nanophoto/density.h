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
  
  $Id: test_problems.h 1856 2010-04-06 14:03:52Z mgr522 $
*/

/** \file density.h
    \brief 

*/

#ifndef MADNESS_INTERIOR_BC_DENSITY_H__INCLUDED
#define MADNESS_INTERIOR_BC_DENSITY_H__INCLUDED

#define WORLD_INSTANTIATE_STATIC_TEMPLATES
#include <mra/mra.h>
#include <mra/lbdeux.h>
#include <mra/sdf_shape_3D.h>
#include <string>

using namespace madness;

enum FunctorOutput { SURFACE, DIRICHLET_RHS, DOMAIN_MASK, DENSITY };

// load balancing structure lifted from dataloadbal.cc
template <int NDIM>
struct DirichletLBCost {
    double leaf_value;
    double parent_value;
    DirichletLBCost(double leaf_value = 1.0, double parent_value = 1.0)
        : leaf_value(leaf_value), parent_value(parent_value) {}

    double operator() (const Key<NDIM> &key,
        const FunctionNode<double, NDIM> &node) const {

        if(key.level() <= 1) {
            return 100.0*(leaf_value+parent_value);
        }
        else if(node.is_leaf()) {
            return leaf_value;
        }
        else {
            return parent_value;
        }
    }
};

/** \brief Setup the tip-molecule problem. */
class TipMolecule : public FunctionFunctorInterface<double, 3> {
    private:
        TipMolecule() : dmi(1.0), denscoeffs(Tensor<double>()),
            basis(std::vector<BasisFunc>(0)) {}

    protected:
        GaussianDomainMask dmi;
        SignedDFInterface<3> *tip, *solid;
        double penalty_prefact, eps;
        int dda_init_level, dens_init_level;
        const Tensor<double> &denscoeffs;
        const std::vector<BasisFunc> &basis;
        std::vector<Vector<double, 3> > specpts;
        double phi, d;

    public:
        /// which function to use when projecting:
        /// -# the weighted surface (SURFACE)
        /// -# the rhs of the auxiliary DE (DIRICHLET_RHS)
        /// -# the domain mask (DOMAIN_MASK)
        /// -# the molecular density (DENSITY)
        FunctorOutput fop;

        /// \brief Sets up the data for the problem-inspecific parts.
        TipMolecule(double eps, double penalty,
            const Tensor<double> &denscoeffs, const std::vector<Atom*> &atoms,
            const std::vector<BasisFunc> &basis, double phi, double d)
            : dmi(eps), tip(NULL), solid(NULL), penalty_prefact(penalty),
              eps(eps), denscoeffs(denscoeffs), basis(basis),
              specpts(0), phi(phi), d(d), fop(DIRICHLET_RHS) {

            // calculate some nice initial projection level
            // should be no lower than 6, but may need to be higher for small
            // length scales

            // epsilon from the diffuse domain approximation
            dda_init_level = ceil(log(6614.0 / eps) / log(2.0) - 4);
            if(dda_init_level < 6)
                dda_init_level = 6;

            // smallest length scale from the molecular density
            dens_init_level = ceil(log(6614.0 / sqrt(0.5 / 18.731137))
                / log(2.0) - 4);
            if(dens_init_level < 6)
                dens_init_level = 6;

            // make the list of special points for the atoms
            for(std::vector<Atom*>::const_iterator iter = atoms.begin();
                iter != atoms.end(); ++iter) {

                specpts.push_back((*iter)->getCenter());
            }

            // make the sdfs
            coord_3d normal, point;

            // solid surface is the xy-plane
            normal[0] = normal[1] = 0.0;
            normal[2] = -1.0;
            point[0] = point[1] = point[2] = 0.0;
            solid = new SDFPlane(normal, point);

            // tip apex is at (0, 0, d)
            point[2] = d;
            normal[2] = 1.0;
            tip = new SDFParaboloid(25.0 / 0.052918, point, normal);
        }

        virtual ~TipMolecule() {
            if(solid != NULL)
                delete solid;
            if(tip != NULL)
                delete tip;
        }

        /// \brief The operator for projecting a MADNESS function.
        double operator() (const Vector<double, 3> &x) const {
            switch(fop) {
            case DIRICHLET_RHS:
                return dmi.mask(solid->sdf(x)) * dmi.mask(-tip->sdf(x))
                    * Inhomogeneity(x) - DirichletCond(x) *
                    (dmi.surface(solid->sdf(x)) + dmi.surface(-tip->sdf(x))) *
                    penalty_prefact;
                break;
            case SURFACE:
                return (dmi.surface(solid->sdf(x)) + dmi.surface(-tip->sdf(x)))
                    * penalty_prefact;
                break;
            case DOMAIN_MASK:
                return dmi.mask(solid->sdf(x)) * dmi.mask(-tip->sdf(x));
                break;
            case DENSITY:
                return Inhomogeneity(x);
                break;
            default:
                error("shouldn't be here...");
                return 0.0;
                break;
            }
        }

        virtual double DirichletCond(const Vector<double, 3> &x) const {
            if(x[2] > 0.5*d)
                return phi;
            else
                return 0.0;
        }

        virtual double Inhomogeneity(const Vector<double, 3> &x) const {
            // all density is close to the (0,0,5ang) for this problem
            if(x[0]*x[0] + x[1]*x[1] +
                (x[2]-5.0/0.052918)*(x[2]-5.0/0.052918) > 100.0)
                return 0.0;

            double ret = 0.0;
            double perstate;

            // go through the states
            long nstate = denscoeffs.dim(0);
            long nbasis = denscoeffs.dim(1);
            for(long state = 0; state < nstate; ++state) {
                perstate = 0.0;

                // go through the coefficients
                for(long func = 0; func < nbasis; ++func) {
                    perstate += denscoeffs(state, func) *
                        basis[func]->operator()(x);
                }

                ret += perstate * perstate;
            }

            if(ret < 1.0e-8)
                return 0.0;
            return 2.0*ret; // 2 for spin degeneracy
        }

        std::vector<Vector<double, 3> > special_points() const {
            if(fop == DOMAIN_MASK) {
                return std::vector<Vector<double, 3> >();
            }
            else if(fop == SURFACE) {
                std::vector<Vector<double, 3> > vec;
                Vector<double, 3> pt;

                pt[0] = pt[1] = pt[2] = 0.0;
                //vec.push_back(pt);

                pt[2] = d;
                //vec.push_back(pt);
                return vec;
            }
            else
                return specpts;
        }

        Level special_level() {
            if(fop == DOMAIN_MASK || fop == SURFACE) {
                return dda_init_level;
            }
            else if(fop == DENSITY) {
                return dens_init_level;
            }
            else {
                if(dens_init_level > dda_init_level)
                    return dens_init_level;
                else
                    return dda_init_level;
            }
        }
};

/** \brief The operator needed for solving for \f$u\f$ with GMRES */
template<int NDIM>
class DirichletCondIntOp : public Operator<Function<double, NDIM> > {
    protected:
        /// \brief The Green's function
        const SeparatedConvolution<double, NDIM> &G;
        /// \brief The surface function (normalized)
        const Function<double, NDIM> &b;

        /** \brief Applies the operator to \c invec

            \note \c G is actually \f$-G\f$.

            \param[in] invec The input vector
            \param[out] outvec The action of the operator on \c invec */
        void action(const Function<double, NDIM> &invec,
                    Function<double, NDIM> &outvec) const {

                outvec = invec + G(b*invec);
                outvec.scale(-1.0);
                outvec.truncate();
        }

    public:
        DirichletCondIntOp(const SeparatedConvolution<double, NDIM> &gin,
            const Function<double, NDIM> &bin)
            : G(gin), b(bin) {}
};

#endif // MADNESS_INTERIOR_BC_DENSITY_H__INCLUDED
