///////////////////////////////////////////////////////////////////////////////
//
// File Expansion.cpp
//
// For more information, please see: http://www.nektar.info
//
// The MIT License
//
// Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
// Department of Aeronautics, Imperial College London (UK), and Scientific
// Computing and Imaging Institute, University of Utah (USA).
//
// License for the specific language governing rights and limitations under
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Description: File for Expansion routines
//
///////////////////////////////////////////////////////////////////////////////

#include <LibUtilities/Foundations/Interp.h>
#include <LocalRegions/Expansion.h>
#include <LocalRegions/MatrixKey.h>

#include <SpatialDomains/MeshComponents.h>
#include <SpatialDomains/CrvTetGeom.h>
#include <SpatialDomains/CrvTriGeom.h>
#include <SpatialDomains/CrvSegGeom.h>

#include <CrvTet.h>
#include <CrvTri2.h>
#include <CrvEdge.h>
#include <LibUtilities/Foundations/ManagerAccess.h>

namespace Nektar
{
    namespace LocalRegions 
    {
        Expansion::Expansion(SpatialDomains::GeometrySharedPtr pGeom) :
                    m_geom(pGeom),
                    m_metricinfo(m_geom->GetGeomFactors())
        {
            if (!m_metricinfo)
            {
                return;
            }
            if (!m_metricinfo->IsValid())
            {
                int nDim = m_base.num_elements();
                string type = "regular";
                if (m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
                {
                    type = "deformed";
                }

                stringstream err;
                err << nDim << "D " << type << " Jacobian not positive "
                    << "(element ID = " << m_geom->GetGlobalID() << ") "
                    << "(first vertex ID = " << m_geom->GetVid(0) << ")";
                NEKERROR(ErrorUtil::ewarning, err.str());
            }
        }
        
        Expansion::Expansion(const Expansion &pSrc) :
                m_geom(pSrc.m_geom),
                m_metricinfo(pSrc.m_metricinfo)
        {

        }

        Expansion::~Expansion()
        {
        }

        DNekScalMatSharedPtr Expansion::GetLocMatrix(const LocalRegions::MatrixKey &mkey)
        {
            return v_GetLocMatrix(mkey);
        }

        DNekMatSharedPtr Expansion::BuildTransformationMatrix(
            const DNekScalMatSharedPtr &r_bnd, 
            const StdRegions::MatrixType matrixType)
        {
            return v_BuildTransformationMatrix(r_bnd,matrixType);
        }

        
        DNekMatSharedPtr Expansion::BuildVertexMatrix(
            const DNekScalMatSharedPtr &r_bnd)
        {
            return v_BuildVertexMatrix(r_bnd);
        }

        void Expansion::AddEdgeNormBoundaryInt(
            const int                           edge,
            const boost::shared_ptr<Expansion> &EdgeExp,
            const Array<OneD, const NekDouble> &Fx,
            const Array<OneD, const NekDouble> &Fy,
                  Array<OneD,       NekDouble> &outarray)
        {
            v_AddEdgeNormBoundaryInt(edge, EdgeExp, Fx, Fy, outarray);
        }

        void Expansion::AddEdgeNormBoundaryInt(
            const int                           edge,
            const boost::shared_ptr<Expansion> &EdgeExp,
            const Array<OneD, const NekDouble> &Fn,
                  Array<OneD,       NekDouble> &outarray)
        {
            v_AddEdgeNormBoundaryInt(edge, EdgeExp, Fn, outarray);
        }

        void Expansion::AddFaceNormBoundaryInt(
            const int                           face,
            const boost::shared_ptr<Expansion> &FaceExp,
            const Array<OneD, const NekDouble> &Fn,
                  Array<OneD,       NekDouble> &outarray)
        {
            v_AddFaceNormBoundaryInt(face, FaceExp, Fn, outarray);
        }

        void Expansion::DGDeriv(
            const int                                   dir,
            const Array<OneD, const NekDouble>&         inarray,
                  Array<OneD, ExpansionSharedPtr>      &EdgeExp,
                  Array<OneD, Array<OneD, NekDouble> > &coeffs,
                  Array<OneD,             NekDouble>   &outarray)
        {
            v_DGDeriv(dir, inarray, EdgeExp, coeffs, outarray);
        }

        DNekScalMatSharedPtr Expansion::GetLocMatrix(const StdRegions::MatrixType mtype,
                    const StdRegions::ConstFactorMap &factors,
                    const StdRegions::VarCoeffMap &varcoeffs)
        {
            MatrixKey mkey(mtype, DetShapeType(), *this, factors, varcoeffs);
            return GetLocMatrix(mkey);
        }

        SpatialDomains::GeometrySharedPtr Expansion::GetGeom() const
        {
            return m_geom;
        }

        const SpatialDomains::GeomFactorsSharedPtr& Expansion::v_GetMetricInfo() const
        {
            return m_metricinfo;
        }


        DNekScalMatSharedPtr Expansion::v_GetLocMatrix(const LocalRegions::MatrixKey &mkey)
        {
            NEKERROR(ErrorUtil::efatal, "This function is only valid for LocalRegions");
            return NullDNekScalMatSharedPtr;
        }

        void Expansion::v_MultiplyByQuadratureMetric(const Array<OneD, const NekDouble>& inarray,
                                                 Array<OneD, NekDouble> &outarray)
        {
            const int nqtot = GetTotPoints();

            if (m_metrics.count(MetricQuadrature) == 0)
            {
                ComputeQuadratureMetric();
            }

            Vmath::Vmul(nqtot, m_metrics[MetricQuadrature], 1, inarray, 1, outarray, 1);
        }

        void Expansion::ComputeLaplacianMetric()
        {
            v_ComputeLaplacianMetric();
        }

        void Expansion::ComputeQuadratureMetric()
        {
            unsigned int nqtot = GetTotPoints();
            SpatialDomains::GeomType type = m_metricinfo->GetGtype();
            LibUtilities::PointsKeyVector p = GetPointsKeys();
            if (type == SpatialDomains::eRegular ||
                   type == SpatialDomains::eMovingRegular)
            {
                m_metrics[MetricQuadrature] = Array<OneD, NekDouble>(nqtot, m_metricinfo->GetJac(p)[0]);
            }
            else
            {
                m_metrics[MetricQuadrature] = m_metricinfo->GetJac(p);
            }

            MultiplyByStdQuadratureMetric(m_metrics[MetricQuadrature],
                                                   m_metrics[MetricQuadrature]);
        }

        void Expansion::v_GetCoords(
            Array<OneD, NekDouble> &coords_0,
            Array<OneD, NekDouble> &coords_1,
            Array<OneD, NekDouble> &coords_2)
        {
            ASSERTL1(m_geom, "m_geom not defined");

            SpatialDomains::CrvSegGeomSharedPtr  CrvSegmentGeom;
            SpatialDomains::CrvTriGeomSharedPtr  CrvTriangleGeom;
            SpatialDomains::CrvTetGeomSharedPtr  CrvTetrahedronGeom;
            bool isCrvTetGeom = false;
            bool isCrvTriGeom = false;
            bool isCrvSegGeom = false;
            CrvTet  * crv_tet_ptr = NULL;
            CrvFace * crv_tri_ptr = NULL;
            CrvEdge * crv_edge_ptr= NULL;

            if ((CrvTetrahedronGeom = boost::dynamic_pointer_cast<SpatialDomains::CrvTetGeom>(m_geom))){
              isCrvTetGeom = true;
              crv_tet_ptr = CrvTetrahedronGeom->get_crv_tet();
            }

            if ((CrvTriangleGeom = boost::dynamic_pointer_cast<SpatialDomains::CrvTriGeom>(m_geom))){
              isCrvTriGeom = true;
              crv_tri_ptr = CrvTriangleGeom->get_crv_tri();
            } 

            if ((CrvSegmentGeom = boost::dynamic_pointer_cast<SpatialDomains::CrvSegGeom>(m_geom))){
              isCrvSegGeom = true;
              crv_edge_ptr = CrvSegmentGeom->get_crv_edge();
            }

            // get physical points defined in Geom
            m_geom->FillGeom();

            const int expDim = m_base.num_elements();
            int       nqGeom = 1;
            bool      doCopy = true;

            Array<OneD, LibUtilities::BasisSharedPtr> CBasis(expDim);
            Array<OneD, Array<OneD, NekDouble> > tmp(3);

            for (int i = 0; i < expDim; ++i)
            {
                CBasis[i] = m_geom->GetBasis(i);
                nqGeom   *= CBasis[i]->GetNumPoints();
                doCopy    = doCopy && m_base[i]->GetBasisKey().SamePoints(
                                                      CBasis[i]->GetBasisKey());
            }

            tmp[0] = coords_0;
            tmp[1] = coords_1;
            tmp[2] = coords_2;

            // doCopy = false;

            if (doCopy)
            {
                for (int i = 0; i < m_geom->GetCoordim(); ++i)
                {
                    m_geom->GetXmap()->BwdTrans(m_geom->GetCoeffs(i), tmp[i]);
                }
            }
            else
            {
              if(crv_tet_ptr) {

                int tmpNumPts0 = m_base[0]->GetPointsKey().GetNumPoints();
                Array<OneD, NekDouble> keyPts0(tmpNumPts0);
                LibUtilities::PointsManager()[m_base[0]->GetPointsKey()]->GetPoints(keyPts0);

                int tmpNumPts1 = m_base[1]->GetPointsKey().GetNumPoints();
                Array<OneD, NekDouble> keyPts1(tmpNumPts1);
                LibUtilities::PointsManager()[m_base[1]->GetPointsKey()]->GetPoints(keyPts1);

                int tmpNumPts2 = m_base[2]->GetPointsKey().GetNumPoints();
                Array<OneD, NekDouble> keyPts2(tmpNumPts2);
                LibUtilities::PointsManager()[m_base[2]->GetPointsKey()]->GetPoints(keyPts2);

                for(int i = 0; i < tmpNumPts2; ++i) // eta3 dir
                {
                    for(int j = 0; j < tmpNumPts1; ++j) // eta2 dir
                    {
                        for(int k = 0; k < tmpNumPts0; ++k) // eta1 dir
                        {
                            Point3d in_eta(keyPts0[k],
                                           keyPts1[j],
                                           keyPts2[i]);
                            Point3d out;
                            crv_tet_ptr->eval_by_coll_eta(in_eta, out);
                            int qindex = i * tmpNumPts0 * tmpNumPts1 + j * tmpNumPts0 + k;
                            tmp[0][qindex] = out[0];
                            tmp[1][qindex] = out[1];
                            tmp[2][qindex] = out[2];
                        }
                    }
                }
              }
              else if(crv_tri_ptr) {
                int tmpNumPts0 = m_base[0]->GetPointsKey().GetNumPoints();
                Array<OneD, NekDouble> keyPts0(tmpNumPts0);
                LibUtilities::PointsManager()[m_base[0]->GetPointsKey()]->GetPoints(keyPts0);
                int tmpNumPts1 = m_base[1]->GetPointsKey().GetNumPoints();
                Array<OneD, NekDouble> keyPts1(tmpNumPts1);
                LibUtilities::PointsManager()[m_base[1]->GetPointsKey()]->GetPoints(keyPts1);
                for(int i = 0; i < tmpNumPts1; ++i) // eta2 dir
                {
                  for(int j = 0; j < tmpNumPts0; ++j) // eta1 dir
                  {
                    Point3d in_eta(keyPts0[j], keyPts1[i], 0.0);
                    Point3d out;
                    crv_tri_ptr->eval_by_coll_eta(in_eta, out);
                    tmp[0][i * tmpNumPts0 + j] = out[0];
                    tmp[1][i * tmpNumPts0 + j] = out[1];
                    tmp[2][i * tmpNumPts0 + j] = out[2];
                  }
                }

              }
              else if (crv_edge_ptr) {
                int tmpNumPts0 = m_base[0]->GetPointsKey().GetNumPoints();
                Array<OneD, NekDouble> keyPts0(tmpNumPts0);
                LibUtilities::PointsManager()[m_base[0]->GetPointsKey()]->GetPoints(keyPts0);
                for(int j = 0; j < tmpNumPts0; ++j) // eta1 dir
                {
                  double xi = -0.5 * keyPts0[j] + 0.5;
                  Point3d in, out;
                  in = Point3d(xi, 0.0, 0.0);
                  crv_edge_ptr->eval_by_xi(in, out);
                  tmp[0][j] = out[0];
                  tmp[1][j] = out[1];
                  tmp[2][j] = out[2];
                }
              }
              else {
                for (int i = 0; i < m_geom->GetCoordim(); ++i)
                {
                    Array<OneD, NekDouble> tmpGeom(nqGeom);
                    m_geom->GetXmap()->BwdTrans(m_geom->GetCoeffs(i), tmpGeom);

                    switch (expDim)
                    {
                        case 1:
                        {
                            LibUtilities::Interp1D(
                                CBasis[0]->GetPointsKey(), &tmpGeom[0],
                                m_base[0]->GetPointsKey(), &tmp[i][0]);
                            break;
                        }
                        case 2:
                        {
                            LibUtilities::Interp2D(
                                CBasis[0]->GetPointsKey(),
                                CBasis[1]->GetPointsKey(),
                                &tmpGeom[0],
                                m_base[0]->GetPointsKey(),
                                m_base[1]->GetPointsKey(),
                                &tmp[i][0]);
                            break;
                        }
                        case 3:
                        {
                            LibUtilities::Interp3D(
                                CBasis[0]->GetPointsKey(),
                                CBasis[1]->GetPointsKey(),
                                CBasis[2]->GetPointsKey(),
                                &tmpGeom[0],
                                m_base[0]->GetPointsKey(),
                                m_base[1]->GetPointsKey(),
                                m_base[2]->GetPointsKey(),
                                &tmp[i][0]);
                            break;
                        }
                    }
                }
              }
            }
        }

        DNekMatSharedPtr Expansion::v_BuildTransformationMatrix(
            const DNekScalMatSharedPtr &r_bnd, 
            const StdRegions::MatrixType matrixType)
        {
            NEKERROR(ErrorUtil::efatal, "This function is only valid for LocalRegions");
            return NullDNekMatSharedPtr;
        }

        DNekMatSharedPtr Expansion::v_BuildVertexMatrix(
            const DNekScalMatSharedPtr &r_bnd)
        {
            NEKERROR(ErrorUtil::efatal, "This function is only valid for LocalRegions");
            return NullDNekMatSharedPtr;
        }

        void Expansion::v_AddEdgeNormBoundaryInt(
            const int                           edge,
            const boost::shared_ptr<Expansion> &EdgeExp,
            const Array<OneD, const NekDouble> &Fx,
            const Array<OneD, const NekDouble> &Fy,
                  Array<OneD,       NekDouble> &outarray)
        {
            NEKERROR(ErrorUtil::efatal, "This function is only valid for LocalRegions");
        }

        void Expansion::v_AddEdgeNormBoundaryInt(
            const int                           edge,
            const boost::shared_ptr<Expansion> &EdgeExp,
            const Array<OneD, const NekDouble> &Fn,
                  Array<OneD,       NekDouble> &outarray)
        {
            NEKERROR(ErrorUtil::efatal, "This function is only valid for LocalRegions");
        }

        void Expansion::v_AddFaceNormBoundaryInt(
            const int                           face,
            const boost::shared_ptr<Expansion> &FaceExp,
            const Array<OneD, const NekDouble> &Fn,
                  Array<OneD,       NekDouble> &outarray)
        {
            NEKERROR(ErrorUtil::efatal, "This function is only valid for LocalRegions");
        }

        void Expansion::v_DGDeriv(
            const int                                   dir,
            const Array<OneD, const NekDouble>&         inarray,
                  Array<OneD, ExpansionSharedPtr>      &EdgeExp,
                  Array<OneD, Array<OneD, NekDouble> > &coeffs,
                  Array<OneD,             NekDouble>   &outarray)
        {
            NEKERROR(ErrorUtil::efatal, "This function is only valid for LocalRegions");
        }
    } //end of namespace
} //end of namespace

