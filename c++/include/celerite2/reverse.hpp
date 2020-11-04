#ifndef _CELERITE2_REVERSE_HPP_DEFINED_
#define _CELERITE2_REVERSE_HPP_DEFINED_

#include <Eigen/Core>
#include "internal.hpp"

namespace celerite2 {
namespace core {

template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename Work, typename InputOut, typename CoeffsOut, typename DiagOut,
          typename LowRankOut>
void factor_rev(const Eigen::MatrixBase<Input> &t,           // (N,)
                const Eigen::MatrixBase<Coeffs> &c,          // (J,)
                const Eigen::MatrixBase<Diag> &a,            // (N,)
                const Eigen::MatrixBase<LowRank> &U,         // (N, J)
                const Eigen::MatrixBase<LowRank> &V,         // (N, J)
                const Eigen::MatrixBase<Diag> &d,            // (N,)
                const Eigen::MatrixBase<LowRank> &W,         // (N, J)
                const Eigen::MatrixBase<Work> &S,            // (N, J*J)
                const Eigen::MatrixBase<Diag> &bd,           // (N,)
                const Eigen::MatrixBase<LowRank> &bW,        // (N, J)
                Eigen::MatrixBase<InputOut> const &bt_out,   // (N,)
                Eigen::MatrixBase<CoeffsOut> const &bc_out,  // (J,)
                Eigen::MatrixBase<DiagOut> const &ba_out,    // (N,)
                Eigen::MatrixBase<LowRankOut> const &bU_out, // (N, J)
                Eigen::MatrixBase<LowRankOut> const &bV_out  // (N, J)

) {
  UNUSED(a);
  UNUSED(V);

  ASSERT_ROW_MAJOR(Work);

  typedef typename Diag::Scalar Scalar;
  typedef typename Eigen::Matrix<Scalar, LowRank::ColsAtCompileTime, LowRank::ColsAtCompileTime> Inner;
  typedef typename Eigen::internal::plain_col_type<Coeffs>::type CoeffVector;

  Eigen::Index N = U.rows(), J = U.cols();
  CAST_VEC(InputOut, bt, N);
  CAST_VEC(CoeffsOut, bc, J);
  CAST_VEC(DiagOut, ba, N);
  CAST_MAT(LowRankOut, bU, N, J);
  CAST_MAT(LowRankOut, bV, N, J);

  // Make local copies of the gradients that we need
  Inner Sn(J, J), bS(J, J);
  Eigen::Map<typename Eigen::internal::plain_row_type<Work>::type> ptr(Sn.data(), 1, J * J);
  Eigen::Matrix<Scalar, LowRank::ColsAtCompileTime, 1> bSWT;
  CoeffVector p(J), bp(J);

  Scalar dt, factor;
  bS.setZero();
  bt.setZero();
  bc.setZero();
  ba.noalias() = bd;
  bV.noalias() = bW;
  bV.array().colwise() /= d.array();
  for (Eigen::Index n = N - 1; n > 0; --n) {
    dt = t(n - 1) - t(n);
    p  = exp(c.array() * dt);

    ptr = S.row(n);

    // Step 6
    ba(n) -= W.row(n) * bV.row(n).transpose();
    bU.row(n).noalias() = -(bV.row(n) + 2.0 * ba(n) * U.row(n)) * Sn * p.asDiagonal();
    bS.noalias() -= U.row(n).transpose() * (bV.row(n) + ba(n) * U.row(n));

    // Step 4
    bp.array() = (bS * Sn + Sn.transpose() * bS).diagonal().array() * p.array();
    bc.noalias() += dt * bp;
    factor = (c.array() * bp.array()).sum();
    bt(n) -= factor;
    bt(n - 1) += factor;

    // Step 3
    bS   = p.asDiagonal() * bS * p.asDiagonal();
    bSWT = bS * W.row(n - 1).transpose();
    ba(n - 1) += W.row(n - 1) * bSWT;
    bV.row(n - 1).noalias() += W.row(n - 1) * (bS + bS.transpose());
  }

  bU.row(0).setZero();
  ba(0) -= bV.row(0) * W.row(0).transpose();
}

template <typename Diag, typename Input, typename Coeffs, typename LowRank, typename RightHandSide, typename Work, typename InputOut,
          typename CoeffsOut, typename LowRankOut, typename DiagOut, typename RightHandSideOut>
void solve_rev(const Eigen::MatrixBase<Input> &t,                // (N,)
               const Eigen::MatrixBase<Coeffs> &c,               // (J,)
               const Eigen::MatrixBase<LowRank> &U,              // (N, J)
               const Eigen::MatrixBase<Diag> &d,                 // (N,)
               const Eigen::MatrixBase<LowRank> &W,              // (N, J)
               const Eigen::MatrixBase<RightHandSide> &Y,        // (N, nrhs)
               const Eigen::MatrixBase<RightHandSide> &X,        // (N, nrhs)
               const Eigen::MatrixBase<RightHandSide> &Z,        // (N, nrhs)
               const Eigen::MatrixBase<Work> &F,                 // (N, J*nrhs)
               const Eigen::MatrixBase<Work> &G,                 // (N, J*nrhs)
               const Eigen::MatrixBase<RightHandSide> &bX,       // (N, nrhs)
               Eigen::MatrixBase<InputOut> const &bt_out,        // (N,)
               Eigen::MatrixBase<CoeffsOut> const &bc_out,       // (J,)
               Eigen::MatrixBase<LowRankOut> const &bU_out,      // (N, J)
               Eigen::MatrixBase<DiagOut> const &bd_out,         // (N,)
               Eigen::MatrixBase<LowRankOut> const &bW_out,      // (N, J)
               Eigen::MatrixBase<RightHandSideOut> const &bY_out // (N, nrhs)
) {
  ASSERT_ROW_MAJOR(Work);

  Eigen::Index N = U.rows(), J = U.cols();
  CAST_VEC(InputOut, bt, N);
  CAST_VEC(CoeffsOut, bc, J);
  CAST_MAT(LowRankOut, bU, N, J);
  CAST_BASE(DiagOut, bd);
  CAST_MAT(LowRankOut, bW, N, J);
  CAST_BASE(RightHandSideOut, bY);

  bt.setZero();
  bc.setZero();
  bU.setZero();
  bW.setZero();

  bY = bX;
  internal::backward_rev<true>(t, c, U, W, Z, X, G, bY, bt, bc, bU, bW, bY);

  bd = -(bY * Z.transpose()).diagonal().array() / d.array().pow(2);
  bY.array().colwise() /= d.array();

  internal::forward_rev<true>(t, c, U, W, Y, Z, F, bY, bt, bc, bU, bW, bY);
}

template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename RightHandSide, typename Norm, typename Work, typename InputOut,
          typename CoeffsOut, typename LowRankOut, typename DiagOut, typename RightHandSideOut>
void norm_rev(const Eigen::MatrixBase<Input> &t,                // (N,)
              const Eigen::MatrixBase<Coeffs> &c,               // (J,)
              const Eigen::MatrixBase<LowRank> &U,              // (N, J)
              const Eigen::MatrixBase<Diag> &d,                 // (N,)
              const Eigen::MatrixBase<LowRank> &W,              // (N, J)
              const Eigen::MatrixBase<RightHandSide> &Y,        // (N, nrhs)
              const Eigen::MatrixBase<Norm> &X,                 // (nrhs, nrhs)
              const Eigen::MatrixBase<RightHandSide> &Z,        // (N, nrhs)
              const Eigen::MatrixBase<Work> &F,                 // (N, J*nrhs)
              const Eigen::MatrixBase<Norm> &bX,                // (nrhs, nrhs)
              Eigen::MatrixBase<InputOut> const &bt_out,        // (N,)
              Eigen::MatrixBase<CoeffsOut> const &bc_out,       // (J,)
              Eigen::MatrixBase<LowRankOut> const &bU_out,      // (N, J)
              Eigen::MatrixBase<DiagOut> const &bd_out,         // (N,)
              Eigen::MatrixBase<LowRankOut> const &bW_out,      // (N, J)
              Eigen::MatrixBase<RightHandSideOut> const &bY_out // (N, nrhs)
) {
  UNUSED(X);

  ASSERT_ROW_MAJOR(Work);

  Eigen::Index N = U.rows(), J = U.cols();
  CAST_VEC(InputOut, bt, N);
  CAST_VEC(CoeffsOut, bc, J);
  CAST_MAT(LowRankOut, bU, N, J);
  CAST_BASE(DiagOut, bd);
  CAST_MAT(LowRankOut, bW, N, J);
  CAST_BASE(RightHandSideOut, bY);

  bt.setZero();
  bc.setZero();
  bU.setZero();
  bW.setZero();

  bY = d.asDiagonal().inverse() * Z * (bX + bX.transpose());
  bd = -(Z * bX * Z.transpose()).diagonal().array() / d.array().pow(2);

  internal::forward_rev<true>(t, c, U, W, Y, Z, F, bY, bt, bc, bU, bW, bY);
}

template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename RightHandSide, typename Work, typename InputOut,
          typename CoeffsOut, typename LowRankOut, typename DiagOut, typename RightHandSideOut>
void dot_tril_rev(const Eigen::MatrixBase<Input> &t,                // (N,)
                  const Eigen::MatrixBase<Coeffs> &c,               // (J,)
                  const Eigen::MatrixBase<LowRank> &U,              // (N, J)
                  const Eigen::MatrixBase<Diag> &d,                 // (N,)
                  const Eigen::MatrixBase<LowRank> &W,              // (N, J)
                  const Eigen::MatrixBase<RightHandSide> &Y,        // (N, nrhs)
                  const Eigen::MatrixBase<RightHandSide> &Z,        // (N, nrhs)
                  const Eigen::MatrixBase<Work> &F,                 // (N, J*nrhs)
                  const Eigen::MatrixBase<RightHandSide> &bZ,       // (N, nrhs)
                  Eigen::MatrixBase<InputOut> const &bt_out,        // (N,)
                  Eigen::MatrixBase<CoeffsOut> const &bc_out,       // (J,)
                  Eigen::MatrixBase<LowRankOut> const &bU_out,      // (N, J)
                  Eigen::MatrixBase<DiagOut> const &bd_out,         // (N,)
                  Eigen::MatrixBase<LowRankOut> const &bW_out,      // (N, J)
                  Eigen::MatrixBase<RightHandSideOut> const &bY_out // (N, nrhs)
) {
  ASSERT_ROW_MAJOR(Work);

  Eigen::Index N = U.rows(), J = U.cols();
  CAST_VEC(InputOut, bt, N);
  CAST_VEC(CoeffsOut, bc, J);
  CAST_BASE(RightHandSideOut, bY);
  CAST_MAT(LowRankOut, bU, N, J);
  CAST_BASE(DiagOut, bd);
  CAST_MAT(LowRankOut, bW, N, J);

  bt.setZero();
  bc.setZero();
  bU.setZero();
  bW.setZero();

  Eigen::Matrix<typename Diag::Scalar, Diag::RowsAtCompileTime, 1> sqrtd = sqrt(d.array());

  // We need to repeat this calculation before running the backprop
  Eigen::Matrix<typename RightHandSide::Scalar, RightHandSide::RowsAtCompileTime, RightHandSide::ColsAtCompileTime, RightHandSide::IsRowMajor> tmp =
     Y;
  tmp.array().colwise() *= sqrtd.array();

  // Run backprop
  bY = bZ;
  internal::forward_rev<false>(t, c, U, W, tmp, Z, F, bZ, bt, bc, bU, bW, bY);

  // Update bY and bd based on tmp op above
  bd = 0.5 * (Y * bY.transpose()).diagonal().array() / sqrtd.array();
  bY.array().colwise() *= sqrtd.array();
}

template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename RightHandSide, typename Work, typename InputOut,
          typename CoeffsOut, typename LowRankOut, typename DiagOut, typename RightHandSideOut>
void matmul_rev(const Eigen::MatrixBase<Input> &t,                // (N,)
                const Eigen::MatrixBase<Coeffs> &c,               // (J,)
                const Eigen::MatrixBase<Diag> &a,                 // (N,)
                const Eigen::MatrixBase<LowRank> &U,              // (N, J)
                const Eigen::MatrixBase<LowRank> &V,              // (N, J)
                const Eigen::MatrixBase<RightHandSide> &Y,        // (N, nrhs)
                const Eigen::MatrixBase<RightHandSide> &X,        // (N, nrhs)
                const Eigen::MatrixBase<RightHandSide> &M,        // (N, nrhs)
                const Eigen::MatrixBase<Work> &F,                 // (N, J*nrhs)
                const Eigen::MatrixBase<Work> &G,                 // (N, J*nrhs)
                const Eigen::MatrixBase<RightHandSide> &bX,       // (N, nrhs)
                Eigen::MatrixBase<InputOut> const &bt_out,        // (N,)
                Eigen::MatrixBase<CoeffsOut> const &bc_out,       // (J,)
                Eigen::MatrixBase<DiagOut> const &ba_out,         // (N,)
                Eigen::MatrixBase<LowRankOut> const &bU_out,      // (N, J)
                Eigen::MatrixBase<LowRankOut> const &bV_out,      // (N, J)
                Eigen::MatrixBase<RightHandSideOut> const &bY_out // (N, nrhs)
) {
  ASSERT_ROW_MAJOR(Work);

  Eigen::Index N = U.rows(), J = U.cols();
  CAST_VEC(InputOut, bt, N);
  CAST_VEC(CoeffsOut, bc, J);
  CAST_BASE(DiagOut, ba);
  CAST_MAT(LowRankOut, bU, N, J);
  CAST_MAT(LowRankOut, bV, N, J);
  CAST_BASE(RightHandSideOut, bY);

  bt.setZero();
  bc.setZero();
  bU.setZero();
  bV.setZero();

  bY = a.asDiagonal() * bX;
  ba = (Y * bX.transpose()).diagonal();

  Eigen::Matrix<typename RightHandSideOut::Scalar, RightHandSideOut::RowsAtCompileTime, RightHandSideOut::ColsAtCompileTime,
                RightHandSideOut::IsRowMajor>
     tmp = bX;

  internal::backward_rev<false>(t, c, U, V, Y, X, G, tmp, bt, bc, bU, bV, bY);
  internal::forward_rev<false>(t, c, U, V, Y, M, F, tmp /* bM */, bt, bc, bU, bV, bY);
}

} // namespace core
} // namespace celerite2

#endif // _CELERITE2_REVERSE_HPP_DEFINED_
