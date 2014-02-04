/**
 * @file lrsdp.hpp
 * @author Ryan Curtin
 *
 * An implementation of Monteiro and Burer's formulation of low-rank
 * semidefinite programs (LR-SDP).
 */
#ifndef __MLPACK_CORE_OPTIMIZERS_LRSDP_LRSDP_HPP
#define __MLPACK_CORE_OPTIMIZERS_LRSDP_LRSDP_HPP

#include <mlpack/core.hpp>
#include <mlpack/core/optimizers/aug_lagrangian/aug_lagrangian.hpp>

namespace mlpack {
namespace optimization {

/**
 * An implementation of a low-rank semidefinite program solver, based on
 * Monteiro and Burer's formulation.  The optimizer is the augmented Lagrangian
 * algorithm, and thus this class has Evaluate(), Gradient(),
 * EvaluateConstraint(), and GradientConstraint() functions for that purpose.
 * However, you should not call the Gradient() or GradientConstraint() functions
 * by hand, because they are only implemented as part of the augmented
 * Lagrangian optimizer.
 */
class LRSDP
{
 public:
  /**
   * Create an LRSDP to be optimized.  The solution will end up being a matrix
   * of size (rank) x (rows).  To construct each constraint and the objective
   * function, use the functions A(), B(), and C() to set them correctly.
   *
   * @param numConstraints Number of constraints in the problem.
   * @param rank Rank of the solution (<= rows).
   * @param rows Number of rows in the solution.
   */
  LRSDP(const size_t numConstraints,
        const arma::mat& initialPoint);

  /**
   * Create an LRSDP to be optimized, passing in an already-created
   * AugLagrangian object.  The given initial point should be set to the size
   * (rows) x (rank), where (rank) is the reduced rank of the problem.
   *
   * @param numConstraints Number of constraints in the problem.
   * @param initialPoint Initial point of the optimization.
   * @param auglag Pre-initialized AugLagrangian<LRSDP> object.
   */
  LRSDP(const size_t numConstraints,
        const arma::mat& initialPoint,
        AugLagrangian<LRSDP>& augLagrangian);

  /**
   * Optimize the LRSDP and return the final objective value.  The given
   * coordinates will be modified to contain the final solution.
   *
   * @param coordinates Starting coordinates for the optimization.
   */
  double Optimize(arma::mat& coordinates);

  /**
   * Evaluate the objective function of the LRSDP (no constraints) at the given
   * coordinates.  This is used by AugLagrangian<LRSDP>.
   */
  double Evaluate(const arma::mat& coordinates) const;

  /**
   * Evaluate the gradient of the LRSDP (no constraints) at the given
   * coordinates.  This is used by AugLagrangian<LRSDP>.
   */
  void Gradient(const arma::mat& coordinates, arma::mat& gradient) const;

  /**
   * Evaluate a particular constraint of the LRSDP at the given coordinates.
   */
  double EvaluateConstraint(const size_t index,
                            const arma::mat& coordinates) const;

  /**
   * Evaluate the gradient of a particular constraint of the LRSDP at the given
   * coordinates.
   */
  void GradientConstraint(const size_t index,
                          const arma::mat& coordinates,
                          arma::mat& gradient) const;

  //! Get the number of constraints in the LRSDP.
  size_t NumConstraints() const { return b.n_elem; }

  //! Get the initial point of the LRSDP.
  const arma::mat& GetInitialPoint();

  //! Return the objective function matrix (C).
  const arma::mat& C() const { return c; }
  //! Modify the objective function matrix (C).
  arma::mat& C() { return c; }

  //! Return the vector of A matrices (which correspond to the constraints).
  const std::vector<arma::mat>& A() const { return a; }
  //! Modify the veector of A matrices (which correspond to the constraints).
  std::vector<arma::mat>& A() { return a; }

  //! Return the vector of modes for the A matrices.
  const arma::uvec& AModes() const { return aModes; }
  //! Modify the vector of modes for the A matrices.
  arma::uvec& AModes() { return aModes; }

  //! Return the vector of B values.
  const arma::vec& B() const { return b; }
  //! Modify the vector of B values.
  arma::vec& B() { return b; }

  //! Return the augmented Lagrangian object.
  const AugLagrangian<LRSDP>& AugLag() const { return augLag; }
  //! Modify the augmented Lagrangian object.
  AugLagrangian<LRSDP>& AugLag() { return augLag; }

  // convert the obkect into a string
  std::string ToString() const;

 private:
  // Should probably use sparse matrices for some of these.

  //! For objective function.
  arma::mat c;
  //! A_i for each constraint.
  std::vector<arma::mat> a;
  //! b_i for each constraint.
  arma::vec b;

  //! 1 if entries in matrix, 0 for normal.
  arma::uvec aModes;

  //! Initial point.
  arma::mat initialPoint;

  //! Internal AugLagrangian object, if one was not passed at construction time.
  AugLagrangian<LRSDP> augLagInternal;

  //! The AugLagrangian object which will be used for optimization.
  AugLagrangian<LRSDP>& augLag;
};

}; // namespace optimization
}; // namespace mlpack

// Include implementation (template specializations for the augmented Lagrangian
// function).
#include "lrsdp_impl.hpp"

#endif

