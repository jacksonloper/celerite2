#ifndef _CELERITE2_INTERFACE_HPP_DEFINED_
#define _CELERITE2_INTERFACE_HPP_DEFINED_

#include <Eigen/Core>
#include "forward.hpp"

namespace celerite2 {
namespace core {

#define MakeEmptyWork(BaseType)                                                                                                                      \
  typedef typename BaseType::Scalar Scalar;                                                                                                          \
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> Empty;                                                              \
  Empty

/**
 * \brief Compute the Cholesky factorization of the system
 *
 * This computes `d` and `W` such that:
 *
 * `diag(a) + tril(U*V^T) + triu(V*U^T) = L*diag(d)*L^T`
 *
 * where
 *
 * `L = 1 + tril(U*W^T)`
 *
 * This can be safely applied in place: `d_out` can point to `a` and `W_out` can
 * point to `V`, and the memory will be reused.
 *
 * @param t     (N,): The input coordinates (must be sorted)
 * @param c     (J,): The transport coefficients
 * @param a     (N,): The diagonal component
 * @param U     (N, J): The first low rank matrix
 * @param V     (N, J): The second low rank matrix
 * @param d_out (N,): The diagonal component of the Cholesky factor
 * @param W_out (N, J): The second low rank component of the Cholesky factor
 */
template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename DiagOut, typename LowRankOut>
Eigen::Index factor(const Eigen::MatrixBase<Input> &t,         // (N,)
                    const Eigen::MatrixBase<Coeffs> &c,        // (J,)
                    const Eigen::MatrixBase<Diag> &a,          // (N,)
                    const Eigen::MatrixBase<LowRank> &U,       // (N, J)
                    const Eigen::MatrixBase<LowRank> &V,       // (N, J)
                    Eigen::MatrixBase<DiagOut> const &d_out,   // (N,)
                    Eigen::MatrixBase<LowRankOut> const &W_out // (N, J)
) {
  MakeEmptyWork(Diag) S;
  return factor<false>(t, c, a, U, V, d_out, W_out, S);
}

/**
 * \brief Solve a linear system using the Cholesky factorization
 *
 * This computes `X` in the following linear system:
 *
 * `K * X = Y`
 *
 * where `K` is the celerite matrix. This uses the results of the Cholesky
 * factorization implemented by `celerite2::core::factor`.
 *
 * This can be safely applied in place by setting `X_out = Y`.
 *
 * @param t     (N,): The input coordinates (must be sorted)
 * @param c     (J,): The transport coefficients
 * @param U     (N, J): The first low rank matrix
 * @param d     (N,): The diagonal component of the Cholesky factor
 * @param W     (N, J): The second low rank component of the Cholesky factor
 * @param Y     (N, Nrhs): The right hand side vector or matrix
 * @param X_out (N, Nrhs): The solution to the linear system
 */
template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename RightHandSide, typename RightHandSideOut>
void solve(const Eigen::MatrixBase<Input> &t,               // (N,)
           const Eigen::MatrixBase<Coeffs> &c,              // (J,)
           const Eigen::MatrixBase<LowRank> &U,             // (N, J)
           const Eigen::MatrixBase<Diag> &d,                // (N,)
           const Eigen::MatrixBase<LowRank> &W,             // (N, J)
           const Eigen::MatrixBase<RightHandSide> &Y,       // (N, nrhs)
           Eigen::MatrixBase<RightHandSideOut> const &X_out // (N, nrhs)
) {
  MakeEmptyWork(Diag) F;
  solve<false>(t, c, U, d, W, Y, X_out, X_out, F, F);
}

/**
 * \brief Compute the norm of vector or matrix under the celerite metric
 *
 * This computes `Y^T * K^-1 * Y` where `K` is the celerite matrix. This uses
 * the results of the Cholesky factorization implemented by
 * `celerite2::core::factor`.
 *
 * This can be safely applied in place by setting `X_out = Y`.
 *
 * @param t     (N,): The input coordinates (must be sorted)
 * @param c     (J,): The transport coefficients
 * @param U     (N, J): The first low rank matrix
 * @param d     (N,): The diagonal component of the Cholesky factor
 * @param W     (N, J): The second low rank component of the Cholesky factor
 * @param Y     (N, Nrhs): The right hand side vector or matrix
 * @param norm_out (Nrhs, Nrhs): The norm of `Y`
 * @param X_out (N, Nrhs): An intermediate result of the operation
 */
template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename RightHandSide, typename Norm, typename RightHandSideOut>
void norm(const Eigen::MatrixBase<Input> &t,               // (N,)
          const Eigen::MatrixBase<Coeffs> &c,              // (J,)
          const Eigen::MatrixBase<LowRank> &U,             // (N, J)
          const Eigen::MatrixBase<Diag> &d,                // (N,)
          const Eigen::MatrixBase<LowRank> &W,             // (N, J)
          const Eigen::MatrixBase<RightHandSide> &Y,       // (N, nrhs)
          Eigen::MatrixBase<Norm> const &norm_out,         // (nrhs, nrhs)
          Eigen::MatrixBase<RightHandSideOut> const &X_out // (N, nrhs)
) {
  MakeEmptyWork(Diag) F;
  norm<false>(t, c, U, d, W, Y, norm_out, X_out, F);
}

/**
 * \brief Compute product of the Cholesky factor with a vector or matrix
 *
 * This computes `L * Y` where `L` is the Cholesky factor of a celerite system
 * computed using `celerite2::core::factor`.
 *
 * This can be safely applied in place by setting `Z_out = Y`.
 *
 * @param t     (N,): The input coordinates (must be sorted)
 * @param c     (J,): The transport coefficients
 * @param U     (N, J): The first low rank matrix
 * @param d     (N,): The diagonal component of the Cholesky factor
 * @param W     (N, J): The second low rank component of the Cholesky factor
 * @param Y     (N, Nrhs): The target vector or matrix
 * @param Z_out (N, Nrhs): The result of the operation
 */
template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename RightHandSide, typename RightHandSideOut>
void dot_tril(const Eigen::MatrixBase<Input> &t,               // (N,)
              const Eigen::MatrixBase<Coeffs> &c,              // (J,)
              const Eigen::MatrixBase<LowRank> &U,             // (N, J)
              const Eigen::MatrixBase<Diag> &d,                // (N,)
              const Eigen::MatrixBase<LowRank> &W,             // (N, J)
              const Eigen::MatrixBase<RightHandSide> &Y,       // (N, nrhs)
              Eigen::MatrixBase<RightHandSideOut> const &Z_out // (N, nrhs)
) {
  MakeEmptyWork(Diag) F;
  dot_tril<false>(t, c, U, d, W, Y, Z_out, F);
}

/**
 * \brief Compute a matrix-vector or matrix-matrix product
 *
 * This computes `X = K * Y` where `K` is the celerite matrix.
 *
 * Note that this operation *cannot* be safely applied in place.
 *
 * @param t      (N,): The input coordinates (must be sorted)
 * @param c      (J,): The transport coefficients
 * @param a      (N,): The diagonal component
 * @param U      (N, J): The first low rank matrix
 * @param V      (N, J): The second low rank matrix
 * @param Y      (N, Nrhs): The matrix that will be left multiplied by the celerite model
 * @param X_out  (N, Nrhs): The result of the operation
 */
template <typename Input, typename Coeffs, typename Diag, typename LowRank, typename RightHandSide, typename RightHandSideOut>
void matmul(const Eigen::MatrixBase<Input> &t,               // (N,)
            const Eigen::MatrixBase<Coeffs> &c,              // (J,)
            const Eigen::MatrixBase<Diag> &a,                // (N,)
            const Eigen::MatrixBase<LowRank> &U,             // (N, J)
            const Eigen::MatrixBase<LowRank> &V,             // (N, J)
            const Eigen::MatrixBase<RightHandSide> &Y,       // (N, nrhs)
            Eigen::MatrixBase<RightHandSideOut> const &X_out // (N, nrhs)
) {
  MakeEmptyWork(Diag) F;
  matmul<false>(t, c, a, U, V, Y, X_out, X_out, F, F);
}

/**
 * \brief The general lower-triangular dot product of a rectangular celerite system
 *
 * @param t1     (N,): The left input coordinates (must be sorted)
 * @param t2     (M,): The right input coordinates (must be sorted)
 * @param c      (J,): The transport coefficients
 * @param U      (N, J): The first low rank matrix
 * @param V      (M, J): The second low rank matrix
 * @param Y      (M, Nrhs): The matrix that will be left multiplied by the celerite model
 * @param Z_out  (N, Nrhs): The result of the operation
 */
template <typename Input, typename Coeffs, typename LowRank, typename RightHandSide, typename RightHandSideOut>
void general_lower_dot(const Eigen::MatrixBase<Input> &t1,              // (N,)
                       const Eigen::MatrixBase<Input> &t2,              // (M,)
                       const Eigen::MatrixBase<Coeffs> &c,              // (J,)
                       const Eigen::MatrixBase<LowRank> &U,             // (N, J)
                       const Eigen::MatrixBase<LowRank> &V,             // (M, J)
                       const Eigen::MatrixBase<RightHandSide> &Y,       // (M, nrhs)
                       Eigen::MatrixBase<RightHandSideOut> const &Z_out // (N, nrhs)
) {
  MakeEmptyWork(Input) F;
  general_lower_dot<false>(t1, t2, c, U, V, Y, Z_out, F);
}

/**
 * \brief The general upper-triangular dot product of a rectangular celerite system
 *
 * @param t1     (N,): The left input coordinates (must be sorted)
 * @param t2     (M,): The right input coordinates (must be sorted)
 * @param c      (J,): The transport coefficients
 * @param U      (N, J): The first low rank matrix
 * @param V      (M, J): The second low rank matrix
 * @param Y      (M, Nrhs): The matrix that will be left multiplied by the celerite model
 * @param Z_out  (N, Nrhs): The result of the operation
 */
template <typename Input, typename Coeffs, typename LowRank, typename RightHandSide, typename RightHandSideOut>
void general_upper_dot(const Eigen::MatrixBase<Input> &t1,              // (N,)
                       const Eigen::MatrixBase<Input> &t2,              // (M,)
                       const Eigen::MatrixBase<Coeffs> &c,              // (J,)
                       const Eigen::MatrixBase<LowRank> &U,             // (N, J)
                       const Eigen::MatrixBase<LowRank> &V,             // (M, J)
                       const Eigen::MatrixBase<RightHandSide> &Y,       // (M, nrhs)
                       Eigen::MatrixBase<RightHandSideOut> const &Z_out // (N, nrhs)
) {
  MakeEmptyWork(Input) F;
  general_upper_dot<false>(t1, t2, c, U, V, Y, Z_out, F);
}

} // namespace core
} // namespace celerite2

#endif // _CELERITE2_INTERFACE_HPP_DEFINED_
