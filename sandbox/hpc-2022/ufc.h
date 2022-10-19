/// This is UFC (Unified Form-assembly Code)
/// This code is released into the public domain.
///
/// The FEniCS Project (http://www.fenicsproject.org/) 2006-2017.
///
/// UFC defines the interface between code generated by FFC
/// and the DOLFIN C++ library. Changes here must be reflected
/// both in the FFC code generation and in the DOLFIN library calls.

#ifndef __UFC_H
#define __UFC_H

#define UFC_VERSION_MAJOR 2019
#define UFC_VERSION_MINOR 1
#define UFC_VERSION_MAINTENANCE 0
#define UFC_VERSION_RELEASE 1

#include <cstddef>
#include <stdexcept>
#include <vector>

#include <ufc_geometry.h>

#define CONCAT(a, b, c) #a "." #b "." #c
#define EVALUATOR(a, b, c) CONCAT(a, b, c)

#if UFC_VERSION_RELEASE
const char UFC_VERSION[] =
    EVALUATOR(UFC_VERSION_MAJOR, UFC_VERSION_MINOR, UFC_VERSION_MAINTENANCE);
#else
const char UFC_VERSION[] = EVALUATOR(
    UFC_VERSION_MAJOR, UFC_VERSION_MINOR, UFC_VERSION_MAINTENANCE) ".dev0";
#endif

#undef CONCAT
#undef EVALUATOR

namespace ufc
{

/// Valid cell shapes
enum class shape
{
  interval,
  triangle,
  quadrilateral,
  tetrahedron,
  hexahedron,
  vertex
};

/// This class defines the data structure for a cell in a mesh.
class cell
{
public:
  /// Shape of the cell
  shape cell_shape;

  /// Topological dimension of the mesh
  std::size_t topological_dimension;

  /// Geometric dimension of the mesh
  std::size_t geometric_dimension;

  /// Array of global indices for the mesh entities of the cell
  std::vector<std::vector<std::size_t>> entity_indices;

  /// Cell index (short-cut for entity_indices[topological_dimension][0])
  std::size_t index;

  /// Local facet index
  int local_facet;

  /// Cell orientation
  int orientation;

  /// Unique mesh identifier
  int mesh_identifier;
};

/// This class defines the interface for a general tensor-valued function.
class function
{
public:
  /// Destructor
  virtual ~function() {}

  /// Evaluate function at given point in cell
  virtual void
  evaluate(double *values, const double *coordinates, const cell &c) const = 0;
};

/// Forward declaration
class coordinate_mapping;

/// This class defines the interface for a finite element.
class finite_element
{
public:
  /// Destructor
  virtual ~finite_element() {}

  /// Return a string identifying the finite element
  virtual const char *signature() const = 0;

  /// Return the cell shape
  virtual shape cell_shape() const = 0;

  /// Return the topological dimension of the cell shape
  virtual std::size_t topological_dimension() const = 0;

  /// Return the geometric dimension of the cell shape
  virtual std::size_t geometric_dimension() const = 0;

  /// Return the dimension of the finite element function space
  virtual std::size_t space_dimension() const = 0;

  /// Return the rank of the value space
  virtual std::size_t value_rank() const = 0;

  /// Return the dimension of the value space for axis i
  virtual std::size_t value_dimension(std::size_t i) const = 0;

  /// Return the number of components of the value space
  virtual std::size_t value_size() const = 0;

  /// Return the rank of the reference value space
  virtual std::size_t reference_value_rank() const = 0;

  /// Return the dimension of the reference value space for axis i
  virtual std::size_t reference_value_dimension(std::size_t i) const = 0;

  /// Return the number of components of the reference value space
  virtual std::size_t reference_value_size() const = 0;

  /// Return the maximum polynomial degree of the finite element function space
  virtual std::size_t degree() const = 0;

  /// Return the family of the finite element function space
  virtual const char *family() const = 0;

  /// Evaluate all basis functions at given point X in reference cell
  ///
  /// @param[out] reference_values
  ///         Basis function values on reference element.
  ///         Dimensions:
  ///         reference_values[num_points][num_dofs][reference_value_size]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  ///
  virtual void evaluate_reference_basis(double *reference_values,
                                        std::size_t num_points,
                                        const double *X) const = 0;

  /// Evaluate specific order derivatives of all basis functions at given point
  /// X in reference cell
  ///
  /// @param[out] reference_values
  ///         Basis function derivative values on reference element.
  ///         Dimensions:
  ///         reference_values[num_points][num_dofs][num_derivatives][reference_value_size]
  ///         where num_derivatives = pow(order, tdim).
  ///         TODO: Document ordering of derivatives for order > 1.
  /// @param[in] order
  ///         Derivative order to compute.
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  ///
  virtual void evaluate_reference_basis_derivatives(double *reference_values,
                                                    std::size_t order,
                                                    std::size_t num_points,
                                                    const double *X) const = 0;

  /// Transform order n derivatives (can be 0) of all basis functions
  /// previously evaluated in points X in reference cell with given
  /// Jacobian J and its inverse K for each point
  ///
  /// @param[out] values
  ///         Transformed basis function (derivative) values.
  ///         Dimensions:
  ///         values[num_points][num_dofs][num_derivatives][value_size] where
  ///         num_derivatives = pow(order, tdim).
  ///         TODO: Document ordering of derivatives for order > 1.
  /// @param[in] order
  ///         Derivative order to compute. Can be zero to just apply Piola
  ///         mappings.
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] reference_values
  ///         Basis function derivative values on reference element.
  ///         Dimensions:
  ///         reference_values[num_points][num_dofs][num_derivatives][reference_value_size]
  ///         where num_derivatives = pow(order, tdim).
  ///         TODO: Document ordering of derivatives for order > 1.
  /// @param[in] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  /// @param[in] J
  ///         Jacobian of coordinate field, J = dx/dX.
  ///         Dimensions: J[num_points][gdim][tdim]
  /// @param[in] detJ
  ///         (Pseudo-)Determinant of Jacobian.
  ///         Dimensions: detJ[num_points]
  /// @param[in] K
  ///         (Pseudo-)Inverse of Jacobian of coordinate field.
  ///         Dimensions: K[num_points][tdim][gdim]
  /// @param[in] cell_orientation
  ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
  ///         Only relevant on manifolds (tdim < gdim).
  ///
  virtual void
  transform_reference_basis_derivatives(double *values,
                                        std::size_t order,
                                        std::size_t num_points,
                                        const double *reference_values,
                                        const double *X,
                                        const double *J,
                                        const double *detJ,
                                        const double *K,
                                        int cell_orientation) const = 0;

  /// Evaluate basis function i at given point x in cell
  virtual void
  evaluate_basis(std::size_t i,
                 double *values,
                 const double *x,
                 const double *coordinate_dofs,
                 int cell_orientation,
                 const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Evaluate all basis functions at given point x in cell
  virtual void
  evaluate_basis_all(double *values,
                     const double *x,
                     const double *coordinate_dofs,
                     int cell_orientation,
                     const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Evaluate order n derivatives of basis function i at given point x in cell
  virtual void evaluate_basis_derivatives(
      std::size_t i,
      std::size_t n,
      double *values,
      const double *x,
      const double *coordinate_dofs,
      int cell_orientation,
      const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Evaluate order n derivatives of all basis functions at given point x in
  /// cell
  virtual void evaluate_basis_derivatives_all(
      std::size_t n,
      double *values,
      const double *x,
      const double *coordinate_dofs,
      int cell_orientation,
      const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Evaluate linear functional for dof i on the function f
  /// The cell argument is only included here so we can pass it to the function.
  virtual double
  evaluate_dof(std::size_t i,
               const function &f,
               const double *coordinate_dofs,
               int cell_orientation,
               const cell &c,
               const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Evaluate linear functionals for all dofs on the function f
  /// The cell argument is only included here so we can pass it to the function.
  virtual void
  evaluate_dofs(double *values,
                const function &f,
                const double *coordinate_dofs,
                int cell_orientation,
                const cell &c,
                const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Interpolate vertex values from dof values
  virtual void interpolate_vertex_values(
      double *vertex_values,
      const double *dof_values,
      const double *coordinate_dofs,
      int cell_orientation,
      const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Tabulate the coordinates of all dofs on a cell
  virtual void tabulate_dof_coordinates(
      double *dof_coordinates,
      const double *coordinate_dofs,
      const ufc::coordinate_mapping *cm = nullptr) const = 0;

  /// Tabulate the coordinates of all dofs on a reference cell
  virtual void tabulate_reference_dof_coordinates(
      double *reference_dof_coordinates) const = 0;

  /// Return the number of sub elements (for a mixed element)
  virtual std::size_t num_sub_elements() const = 0;

  /// Create a new finite element for sub element i (for a mixed element)
  virtual finite_element *create_sub_element(std::size_t i) const = 0;

  /// Create a new class instance
  virtual finite_element *create() const = 0;
};

/// This class defines the interface for a local-to-global mapping of
/// degrees of freedom (dofs).
class dofmap
{
public:
  /// Destructor
  virtual ~dofmap() {}

  /// Return a string identifying the dofmap
  virtual const char *signature() const = 0;

  /// Return true iff mesh entities of topological dimension d are
  /// needed
  virtual bool needs_mesh_entities(std::size_t d) const = 0;

  /// Return the topological dimension of the associated cell shape
  virtual std::size_t topological_dimension() const = 0;

  /// Return the dimension of the global finite element function space
  virtual std::size_t global_dimension(
      const std::vector<std::size_t> &num_global_mesh_entities) const = 0;

  /// Return the dimension of the local finite element function space
  /// Return the number of dofs with global support (i.e. global constants)
  virtual std::size_t num_global_support_dofs() const = 0;

  /// Return the dimension of the local finite element function space
  /// for a cell (not including global support dofs)
  virtual std::size_t num_element_support_dofs() const = 0;

  /// Return the dimension of the local finite element function space
  /// for a cell (old version including global support dofs)
  virtual std::size_t num_element_dofs() const = 0;

  /// Return the number of dofs on each cell facet
  virtual std::size_t num_facet_dofs() const = 0;

  /// Return the number of dofs associated with each cell
  /// entity of dimension d
  virtual std::size_t num_entity_dofs(std::size_t d) const = 0;

  /// Return the number of dofs associated with the closure
  /// of each cell entity dimension d
  virtual std::size_t num_entity_closure_dofs(std::size_t d) const = 0;

  /// Tabulate the local-to-global mapping of dofs on a cell
  virtual void tabulate_dofs(
      std::size_t *dofs,
      const std::vector<std::size_t> &num_global_entities,
      const std::vector<std::vector<std::size_t>> &entity_indices) const = 0;

  /// Tabulate the local-to-local mapping from facet dofs to cell dofs
  virtual void tabulate_facet_dofs(std::size_t *dofs,
                                   std::size_t facet) const = 0;

  /// Tabulate the local-to-local mapping of dofs on entity (d, i)
  virtual void tabulate_entity_dofs(std::size_t *dofs,
                                    std::size_t d,
                                    std::size_t i) const = 0;

  /// Tabulate the local-to-local mapping of dofs on the closure of entity (d,
  /// i)
  virtual void tabulate_entity_closure_dofs(std::size_t *dofs,
                                            std::size_t d,
                                            std::size_t i) const = 0;

  /// Return the number of sub dofmaps (for a mixed element)
  virtual std::size_t num_sub_dofmaps() const = 0;

  /// Create a new dofmap for sub dofmap i (for a mixed element)
  virtual dofmap *create_sub_dofmap(std::size_t i) const = 0;

  /// Create a new class instance
  virtual dofmap *create() const = 0;
};

/// A representation of a coordinate mapping parameterized by a local finite
/// element basis on each cell
class coordinate_mapping
{
public:
  virtual ~coordinate_mapping() {}

  /// Return coordinate_mapping signature string
  virtual const char *signature() const = 0;

  /// Create object of the same type
  virtual coordinate_mapping *create() const = 0;

  /// Return geometric dimension of the coordinate_mapping
  virtual std::size_t geometric_dimension() const = 0;

  /// Return topological dimension of the coordinate_mapping
  virtual std::size_t topological_dimension() const = 0;

  /// Return cell shape of the coordinate_mapping
  virtual shape cell_shape() const = 0;

  /// Create finite_element object representing the coordinate parameterization
  virtual finite_element *create_coordinate_finite_element() const = 0;

  /// Create dofmap object representing the coordinate parameterization
  virtual dofmap *create_coordinate_dofmap() const = 0;

  /// Compute physical coordinates x from reference coordinates X,
  /// the inverse of compute_reference_coordinates
  ///
  /// @param[out] x
  ///         Physical coordinates.
  ///         Dimensions: x[num_points][gdim]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  /// @param[in] coordinate_dofs
  ///         Dofs of the coordinate field on the cell.
  ///         Dimensions: coordinate_dofs[num_dofs][gdim].
  ///
  virtual void
  compute_physical_coordinates(double *x,
                               std::size_t num_points,
                               const double *X,
                               const double *coordinate_dofs) const = 0;

  /// Compute reference coordinates X from physical coordinates x,
  /// the inverse of compute_physical_coordinates
  ///
  /// @param[out] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] x
  ///         Physical coordinates.
  ///         Dimensions: x[num_points][gdim]
  /// @param[in] coordinate_dofs
  ///         Dofs of the coordinate field on the cell.
  ///         Dimensions: coordinate_dofs[num_dofs][gdim].
  /// @param[in] cell_orientation
  ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
  ///         Only relevant on manifolds (tdim < gdim).
  ///
  virtual void compute_reference_coordinates(double *X,
                                             std::size_t num_points,
                                             const double *x,
                                             const double *coordinate_dofs,
                                             int cell_orientation) const = 0;

  /// Compute X, J, detJ, K from physical coordinates x on a cell
  ///
  /// @param[out] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  /// @param[out] J
  ///         Jacobian of coordinate field, J = dx/dX.
  ///         Dimensions: J[num_points][gdim][tdim]
  /// @param[out] detJ
  ///         (Pseudo-)Determinant of Jacobian.
  ///         Dimensions: detJ[num_points]
  /// @param[out] K
  ///         (Pseudo-)Inverse of Jacobian of coordinate field.
  ///         Dimensions: K[num_points][tdim][gdim]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] x
  ///         Physical coordinates.
  ///         Dimensions: x[num_points][gdim]
  /// @param[in] coordinate_dofs
  ///         Dofs of the coordinate field on the cell.
  ///         Dimensions: coordinate_dofs[num_dofs][gdim].
  /// @param[in] cell_orientation
  ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
  ///         Only relevant on manifolds (tdim < gdim).
  ///
  virtual void compute_reference_geometry(double *X,
                                          double *J,
                                          double *detJ,
                                          double *K,
                                          std::size_t num_points,
                                          const double *x,
                                          const double *coordinate_dofs,
                                          int cell_orientation) const = 0;

  /// Compute Jacobian of coordinate mapping J = dx/dX at reference coordinates
  /// X
  ///
  /// @param[out] J
  ///         Jacobian of coordinate field, J = dx/dX.
  ///         Dimensions: J[num_points][gdim][tdim]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  /// @param[in] coordinate_dofs
  ///         Dofs of the coordinate field on the cell.
  ///         Dimensions: coordinate_dofs[num_dofs][gdim].
  ///
  virtual void compute_jacobians(double *J,
                                 std::size_t num_points,
                                 const double *X,
                                 const double *coordinate_dofs) const = 0;

  /// Compute determinants of (pseudo-)Jacobians J
  ///
  /// @param[out] detJ
  ///         (Pseudo-)Determinant of Jacobian.
  ///         Dimensions: detJ[num_points]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] J
  ///         Jacobian of coordinate field, J = dx/dX.
  ///         Dimensions: J[num_points][gdim][tdim]
  /// @param[in] cell_orientation
  ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
  ///         Only relevant on manifolds (tdim < gdim).
  ///
  virtual void compute_jacobian_determinants(double *detJ,
                                             std::size_t num_points,
                                             const double *J,
                                             int cell_orientation) const = 0;

  /// Compute (pseudo-)inverses K of (pseudo-)Jacobians J
  ///
  /// @param[out] K
  ///         (Pseudo-)Inverse of Jacobian of coordinate field.
  ///         Dimensions: K[num_points][tdim][gdim]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] J
  ///         Jacobian of coordinate field, J = dx/dX.
  ///         Dimensions: J[num_points][gdim][tdim]
  /// @param[in] detJ
  ///         (Pseudo-)Determinant of Jacobian.
  ///         Dimensions: detJ[num_points]
  ///
  virtual void compute_jacobian_inverses(double *K,
                                         std::size_t num_points,
                                         const double *J,
                                         const double *detJ) const = 0;

  /// Combined (for convenience) computation of x, J, detJ, K from X and
  /// coordinate_dofs on a cell
  ///
  /// @param[out] x
  ///         Physical coordinates.
  ///         Dimensions: x[num_points][gdim]
  /// @param[out] J
  ///         Jacobian of coordinate field, J = dx/dX.
  ///         Dimensions: J[num_points][gdim][tdim]
  /// @param[out] detJ
  ///         (Pseudo-)Determinant of Jacobian.
  ///         Dimensions: detJ[num_points]
  /// @param[out] K
  ///         (Pseudo-)Inverse of Jacobian of coordinate field.
  ///         Dimensions: K[num_points][tdim][gdim]
  /// @param[in] num_points
  ///         Number of points.
  /// @param[in] X
  ///         Reference cell coordinates.
  ///         Dimensions: X[num_points][tdim]
  /// @param[in] coordinate_dofs
  ///         Dofs of the coordinate field on the cell.
  ///         Dimensions: coordinate_dofs[num_dofs][gdim].
  /// @param[in] cell_orientation
  ///         Orientation of the cell, 1 means flipped w.r.t. reference cell.
  ///         Only relevant on manifolds (tdim < gdim).
  ///
  virtual void compute_geometry(double *x,
                                double *J,
                                double *detJ,
                                double *K,
                                std::size_t num_points,
                                const double *X,
                                const double *coordinate_dofs,
                                int cell_orientation) const = 0;

  /// Compute x and J at midpoint of cell
  ///
  /// @param[out] x
  ///         Physical coordinates.
  ///         Dimensions: x[gdim]
  /// @param[out] J
  ///         Jacobian of coordinate field, J = dx/dX.
  ///         Dimensions: J[gdim][tdim]
  /// @param[in] coordinate_dofs
  ///         Dofs of the coordinate field on the cell.
  ///         Dimensions: coordinate_dofs[num_dofs][gdim].
  ///
  virtual void compute_midpoint_geometry(
      double *x, double *J, const double *coordinate_dofs) const = 0;
};

/// This class defines the shared interface for classes implementing
/// the tabulation of a tensor corresponding to the local contribution
/// to a form from an integral.
class integral
{
public:
  /// Destructor
  virtual ~integral() {}

  /// Tabulate which form coefficients are used by this integral
  virtual const std::vector<bool> &enabled_coefficients() const = 0;
};

/// This class defines the interface for the tabulation of the cell
/// tensor corresponding to the local contribution to a form from
/// the integral over a cell.
class cell_integral : public integral
{
public:
  /// Destructor
  virtual ~cell_integral() {}

  /// Tabulate the tensor for the contribution from a local cell
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs,
                               int cell_orientation) const = 0;
};

/// This class defines the interface for the tabulation of the
/// exterior facet tensor corresponding to the local contribution to
/// a form from the integral over an exterior facet.
class exterior_facet_integral : public integral
{
public:
  /// Destructor
  virtual ~exterior_facet_integral() {}

  /// Tabulate the tensor for the contribution from a local exterior facet
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs,
                               std::size_t facet,
                               int cell_orientation) const = 0;
};

/// This class defines the interface for the tabulation of the
/// interior facet tensor corresponding to the local contribution to
/// a form from the integral over an interior facet.
class interior_facet_integral : public integral
{
public:
  /// Destructor
  virtual ~interior_facet_integral() {}

  /// Tabulate the tensor for the contribution from a local interior facet
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs_0,
                               const double *coordinate_dofs_1,
                               std::size_t facet_0,
                               std::size_t facet_1,
                               int cell_orientation_0,
                               int cell_orientation_1) const = 0;
};

/// This class defines the interface for the tabulation of
/// an expression evaluated at exactly one point.
class vertex_integral : public integral
{
public:
  /// Constructor
  vertex_integral() {}

  /// Destructor
  virtual ~vertex_integral() {}

  /// Tabulate the tensor for the contribution from the local vertex
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs,
                               std::size_t vertex,
                               int cell_orientation) const = 0;
};

/// This class defines the interface for the tabulation of the
/// tensor corresponding to the local contribution to a form from
/// the integral over a custom domain defined in terms of a set of
/// quadrature points and weights.
class custom_integral : public integral
{
public:
  /// Constructor
  custom_integral() {}

  /// Destructor
  virtual ~custom_integral(){};

  /// Return the number of cells involved in evaluation of the integral
  virtual std::size_t num_cells() const = 0;

  /// Tabulate the tensor for the contribution from a custom domain
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs,
                               std::size_t num_quadrature_points,
                               const double *quadrature_points,
                               const double *quadrature_weights,
                               const double *facet_normals,
                               int cell_orientation) const = 0;
};

/// This class defines the interface for the tabulation of the
/// tensor corresponding to the local contribution to a form from
/// the integral over a cut cell defined in terms of a set of
/// quadrature points and weights.
class cutcell_integral : public integral
{
public:
  /// Constructor
  cutcell_integral() {}

  /// Destructor
  virtual ~cutcell_integral(){};

  /// Tabulate the tensor for the contribution from a cutcell domain
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs,
                               std::size_t num_quadrature_points,
                               const double *quadrature_points,
                               const double *quadrature_weights,
                               int cell_orientation) const = 0;
};

/// This class defines the interface for the tabulation of the
/// tensor corresponding to the local contribution to a form from
/// the integral over a cut cell defined in terms of a set of
/// quadrature points and weights.
class interface_integral : public integral
{
public:
  /// Constructor
  interface_integral() {}

  /// Destructor
  virtual ~interface_integral(){};

  /// Tabulate the tensor for the contribution from an interface domain
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs,
                               std::size_t num_quadrature_points,
                               const double *quadrature_points,
                               const double *quadrature_weights,
                               const double *facet_normals,
                               int cell_orientation) const = 0;
};

/// This class defines the interface for the tabulation of the
/// tensor corresponding to the local contribution to a form from
/// the integral over the overlapped portion of a cell defined in
/// terms of a set of quadrature points and weights.
class overlap_integral : public integral
{
public:
  /// Constructor
  overlap_integral() {}

  /// Destructor
  virtual ~overlap_integral(){};

  /// Tabulate the tensor for the contribution from an overlap domain
  virtual void tabulate_tensor(double *A,
                               const double *const *w,
                               const double *coordinate_dofs,
                               std::size_t num_quadrature_points,
                               const double *quadrature_points,
                               const double *quadrature_weights,
                               int cell_orientation) const = 0;
};

/// This class defines the interface for the assembly of the global
/// tensor corresponding to a form with r + n arguments, that is, a
/// mapping
///
///     a : V1 x V2 x ... Vr x W1 x W2 x ... x Wn -> R
///
/// with arguments v1, v2, ..., vr, w1, w2, ..., wn. The rank r
/// global tensor A is defined by
///
///     A = a(V1, V2, ..., Vr, w1, w2, ..., wn),
///
/// where each argument Vj represents the application to the
/// sequence of basis functions of Vj and w1, w2, ..., wn are given
/// fixed functions (coefficients).
class form
{
public:
  /// Destructor
  virtual ~form() {}

  /// Return a string identifying the form
  virtual const char *signature() const = 0;

  /// Return the rank of the global tensor (r)
  virtual std::size_t rank() const = 0;

  /// Return the number of coefficients (n)
  virtual std::size_t num_coefficients() const = 0;

  /// Return original coefficient position for each coefficient
  ///
  /// @param i
  ///        Coefficient number, 0 <= i < n
  ///
  virtual std::size_t original_coefficient_position(std::size_t i) const = 0;

  /// Create a new finite element for parameterization of coordinates
  virtual finite_element *create_coordinate_finite_element() const = 0;

  /// Create a new dofmap for parameterization of coordinates
  virtual dofmap *create_coordinate_dofmap() const = 0;

  /// Create a new coordinate mapping
  virtual coordinate_mapping *create_coordinate_mapping() const = 0;

  /// Create a new finite element for argument function 0 <= i < r+n
  ///
  /// @param i
  ///        Argument number if 0 <= i < r
  ///        Coefficient number j=i-r if r+j <= i < r+n
  ///
  virtual finite_element *create_finite_element(std::size_t i) const = 0;

  /// Create a new dofmap for argument function 0 <= i < r+n
  ///
  /// @param i
  ///        Argument number if 0 <= i < r
  ///        Coefficient number j=i-r if r+j <= i < r+n
  ///
  virtual dofmap *create_dofmap(std::size_t i) const = 0;

  /// Return the upper bound on subdomain ids for cell integrals
  virtual std::size_t max_cell_subdomain_id() const = 0;

  /// Return the upper bound on subdomain ids for exterior facet integrals
  virtual std::size_t max_exterior_facet_subdomain_id() const = 0;

  /// Return the upper bound on subdomain ids for interior facet integrals
  virtual std::size_t max_interior_facet_subdomain_id() const = 0;

  /// Return the upper bound on subdomain ids for vertex integrals
  virtual std::size_t max_vertex_subdomain_id() const = 0;

  /// Return the upper bound on subdomain ids for custom integrals
  virtual std::size_t max_custom_subdomain_id() const = 0;

  /// Return the upper bound on subdomain ids for cutcell integrals
  virtual std::size_t max_cutcell_subdomain_id() const = 0;

  /// Return the upper bound on subdomain ids for interface integrals
  virtual std::size_t max_interface_subdomain_id() const = 0;

  /// Return the upper bound on subdomain ids for overlap integrals
  virtual std::size_t max_overlap_subdomain_id() const = 0;

  /// Return whether form has any cell integrals
  virtual bool has_cell_integrals() const = 0;

  /// Return whether form has any exterior facet integrals
  virtual bool has_exterior_facet_integrals() const = 0;

  /// Return whether form has any interior facet integrals
  virtual bool has_interior_facet_integrals() const = 0;

  /// Return whether form has any vertex integrals
  virtual bool has_vertex_integrals() const = 0;

  /// Return whether form has any custom integrals
  virtual bool has_custom_integrals() const = 0;

  /// Return whether form has any cutcell integrals
  virtual bool has_cutcell_integrals() const = 0;

  /// Return whether form has any interface integrals
  virtual bool has_interface_integrals() const = 0;

  /// Return whether form has any overlap integrals
  virtual bool has_overlap_integrals() const = 0;

  /// Create a new cell integral on sub domain subdomain_id
  virtual cell_integral *
  create_cell_integral(std::size_t subdomain_id) const = 0;

  /// Create a new exterior facet integral on sub domain subdomain_id
  virtual exterior_facet_integral *
  create_exterior_facet_integral(std::size_t subdomain_id) const = 0;

  /// Create a new interior facet integral on sub domain subdomain_id
  virtual interior_facet_integral *
  create_interior_facet_integral(std::size_t subdomain_id) const = 0;

  /// Create a new vertex integral on sub domain subdomain_id
  virtual vertex_integral *
  create_vertex_integral(std::size_t subdomain_id) const = 0;

  /// Create a new custom integral on sub domain subdomain_id
  virtual custom_integral *
  create_custom_integral(std::size_t subdomain_id) const = 0;

  /// Create a new cutcell integral on sub domain subdomain_id
  virtual cutcell_integral *
  create_cutcell_integral(std::size_t subdomain_id) const = 0;

  /// Create a new interface integral on sub domain subdomain_id
  virtual interface_integral *
  create_interface_integral(std::size_t subdomain_id) const = 0;

  /// Create a new overlap integral on sub domain subdomain_id
  virtual overlap_integral *
  create_overlap_integral(std::size_t subdomain_id) const = 0;

  /// Create a new cell integral on everywhere else
  virtual cell_integral *create_default_cell_integral() const = 0;

  /// Create a new exterior facet integral on everywhere else
  virtual exterior_facet_integral *
  create_default_exterior_facet_integral() const = 0;

  /// Create a new interior facet integral on everywhere else
  virtual interior_facet_integral *
  create_default_interior_facet_integral() const = 0;

  /// Create a new vertex integral on everywhere else
  virtual vertex_integral *create_default_vertex_integral() const = 0;

  /// Create a new custom integral on everywhere else
  virtual custom_integral *create_default_custom_integral() const = 0;

  /// Create a new cutcell integral on everywhere else
  virtual cutcell_integral *create_default_cutcell_integral() const = 0;

  /// Create a new interface integral on everywhere else
  virtual interface_integral *create_default_interface_integral() const = 0;

  /// Create a new overlap integral on everywhere else
  virtual overlap_integral *create_default_overlap_integral() const = 0;
};

} // namespace ufc

#endif