// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "open3d/pipelines/registration/Registration.h"

#include "open3d/geometry/PointCloud.h"
#include "open3d/pipelines/registration/ColoredICP.h"
#include "open3d/pipelines/registration/CorrespondenceChecker.h"
#include "open3d/pipelines/registration/FastGlobalRegistration.h"
#include "open3d/pipelines/registration/Feature.h"
#include "open3d/pipelines/registration/TransformationEstimation.h"
#include "open3d/utility/Console.h"
#include "pybind/docstring.h"
#include "pybind/pipelines/registration/registration.h"

namespace open3d {

template <class TransformationEstimationBase =
                  pipelines::registration::TransformationEstimation>
class PyTransformationEstimation : public TransformationEstimationBase {
public:
    using TransformationEstimationBase::TransformationEstimationBase;
    pipelines::registration::TransformationEstimationType
    GetTransformationEstimationType() const override {
        PYBIND11_OVERLOAD_PURE(
                pipelines::registration::TransformationEstimationType,
                TransformationEstimationBase, void);
    }
    double ComputeRMSE(const geometry::PointCloud &source,
                       const geometry::PointCloud &target,
                       const pipelines::registration::CorrespondenceSet &corres)
            const override {
        PYBIND11_OVERLOAD_PURE(double, TransformationEstimationBase, source,
                               target, corres);
    }
    Eigen::Matrix4d ComputeTransformation(
            const geometry::PointCloud &source,
            const geometry::PointCloud &target,
            const pipelines::registration::CorrespondenceSet &corres)
            const override {
        PYBIND11_OVERLOAD_PURE(Eigen::Matrix4d, TransformationEstimationBase,
                               source, target, corres);
    }
};

template <class CorrespondenceCheckerBase =
                  pipelines::registration::CorrespondenceChecker>
class PyCorrespondenceChecker : public CorrespondenceCheckerBase {
public:
    using CorrespondenceCheckerBase::CorrespondenceCheckerBase;
    bool Check(const geometry::PointCloud &source,
               const geometry::PointCloud &target,
               const pipelines::registration::CorrespondenceSet &corres,
               const Eigen::Matrix4d &transformation) const override {
        PYBIND11_OVERLOAD_PURE(bool, CorrespondenceCheckerBase, source, target,
                               corres, transformation);
    }
};

void pybind_registration_classes(py::module &m) {
    // open3d.registration.ICPConvergenceCriteria
    py::class_<pipelines::registration::ICPConvergenceCriteria>
            convergence_criteria(
                    m, "ICPConvergenceCriteria",
                    "Class that defines the convergence criteria of ICP. ICP "
                    "algorithm "
                    "stops if the relative change of fitness and rmse hit "
                    "``relative_fitness`` and ``relative_rmse`` individually, "
                    "or the "
                    "iteration number exceeds ``max_iteration``.");
    py::detail::bind_copy_functions<
            pipelines::registration::ICPConvergenceCriteria>(
            convergence_criteria);
    convergence_criteria
            .def(py::init([](double fitness, double rmse, int itr) {
                     return new pipelines::registration::ICPConvergenceCriteria(
                             fitness, rmse, itr);
                 }),
                 "relative_fitness"_a = 1e-6, "relative_rmse"_a = 1e-6,
                 "max_iteration"_a = 30)
            .def_readwrite(
                    "relative_fitness",
                    &pipelines::registration::ICPConvergenceCriteria::
                            relative_fitness_,
                    "If relative change (difference) of fitness score is lower "
                    "than ``relative_fitness``, the iteration stops.")
            .def_readwrite(
                    "relative_rmse",
                    &pipelines::registration::ICPConvergenceCriteria::
                            relative_rmse_,
                    "If relative change (difference) of inliner RMSE score is "
                    "lower than ``relative_rmse``, the iteration stops.")
            .def_readwrite("max_iteration",
                           &pipelines::registration::ICPConvergenceCriteria::
                                   max_iteration_,
                           "Maximum iteration before iteration stops.")
            .def("__repr__", [](const pipelines::registration::
                                        ICPConvergenceCriteria &c) {
                return fmt::format(
                        "pipelines::registration::ICPConvergenceCriteria class "
                        "with relative_fitness={:e}, relative_rmse={:e}, "
                        "and max_iteration={:d}",
                        c.relative_fitness_, c.relative_rmse_,
                        c.max_iteration_);
            });

    // open3d.registration.RANSACConvergenceCriteria
    py::class_<pipelines::registration::RANSACConvergenceCriteria>
            ransac_criteria(m, "RANSACConvergenceCriteria",
                            "Class that defines the convergence criteria of "
                            "RANSAC. RANSAC algorithm stops if the iteration "
                            "number hits ``max_iteration``, or the validation "
                            "has been run for ``max_validation`` times. Note "
                            "that the validation is the most computational "
                            "expensive operator in an iteration. Most "
                            "iterations do not do full validation. It is "
                            "crucial to control ``max_validation`` so that the "
                            "computation time is acceptable.");
    py::detail::bind_copy_functions<
            pipelines::registration::RANSACConvergenceCriteria>(
            ransac_criteria);
    ransac_criteria
            .def(py::init([](int max_iteration, int max_validation) {
                     return new pipelines::registration::
                             RANSACConvergenceCriteria(max_iteration,
                                                       max_validation);
                 }),
                 "max_iteration"_a = 1000, "max_validation"_a = 1000)
            .def_readwrite("max_iteration",
                           &pipelines::registration::RANSACConvergenceCriteria::
                                   max_iteration_,
                           "Maximum iteration before iteration stops.")
            .def_readwrite(
                    "max_validation",
                    &pipelines::registration::RANSACConvergenceCriteria::
                            max_validation_,
                    "Maximum times the validation has been run before the "
                    "iteration stops.")
            .def("__repr__", [](const pipelines::registration::
                                        RANSACConvergenceCriteria &c) {
                return fmt::format(
                        "pipelines::registration::RANSACConvergenceCriteria "
                        "class with max_iteration={:d}, "
                        "and max_validation={:d}",
                        c.max_iteration_, c.max_validation_);
            });

    // open3d.registration.TransformationEstimation
    py::class_<pipelines::registration::TransformationEstimation,
               PyTransformationEstimation<
                       pipelines::registration::TransformationEstimation>>
            te(m, "TransformationEstimation",
               "Base class that estimates a transformation between two point "
               "clouds. The virtual function ComputeTransformation() must be "
               "implemented in subclasses.");
    te.def("compute_rmse",
           &pipelines::registration::TransformationEstimation::ComputeRMSE,
           "source"_a, "target"_a, "corres"_a,
           "Compute RMSE between source and target points cloud given "
           "correspondences.");
    te.def("compute_transformation",
           &pipelines::registration::TransformationEstimation::
                   ComputeTransformation,
           "source"_a, "target"_a, "corres"_a,
           "Compute transformation from source to target point cloud given "
           "correspondences.");
    docstring::ClassMethodDocInject(
            m, "TransformationEstimation", "compute_rmse",
            {{"source", "Source point cloud."},
             {"target", "Target point cloud."},
             {"corres",
              "Correspondence set between source and target point cloud."}});
    docstring::ClassMethodDocInject(
            m, "TransformationEstimation", "compute_transformation",
            {{"source", "Source point cloud."},
             {"target", "Target point cloud."},
             {"corres",
              "Correspondence set between source and target point cloud."}});

    // open3d.registration.TransformationEstimationPointToPoint:
    // TransformationEstimation
    py::class_<pipelines::registration::TransformationEstimationPointToPoint,
               PyTransformationEstimation<
                       pipelines::registration::
                               TransformationEstimationPointToPoint>,
               pipelines::registration::TransformationEstimation>
            te_p2p(m, "TransformationEstimationPointToPoint",
                   "Class to estimate a transformation for point to point "
                   "distance.");
    py::detail::bind_copy_functions<
            pipelines::registration::TransformationEstimationPointToPoint>(
            te_p2p);
    te_p2p.def(py::init([](bool with_scaling) {
                   return new pipelines::registration::
                           TransformationEstimationPointToPoint(with_scaling);
               }),
               "with_scaling"_a = false)
            .def("__repr__",
                 [](const pipelines::registration::
                            TransformationEstimationPointToPoint &te) {
                     return std::string(
                                    "pipelines::registration::"
                                    "TransformationEstimationPointToPoint ") +
                            (te.with_scaling_
                                     ? std::string("with scaling.")
                                     : std::string("without scaling."));
                 })
            .def_readwrite(
                    "with_scaling",
                    &pipelines::registration::
                            TransformationEstimationPointToPoint::with_scaling_,
                    R"(Set to ``True`` to estimate scaling, ``False`` to force
scaling to be ``1``.

The homogeneous transformation is given by

:math:`T = \begin{bmatrix} c\mathbf{R} & \mathbf{t} \\ \mathbf{0} & 1 \end{bmatrix}`

Sets :math:`c = 1` if ``with_scaling`` is ``False``.
)");

    // open3d.registration.TransformationEstimationPointToPlane:
    // TransformationEstimation
    py::class_<pipelines::registration::TransformationEstimationPointToPlane,
               PyTransformationEstimation<
                       pipelines::registration::
                               TransformationEstimationPointToPlane>,
               pipelines::registration::TransformationEstimation>
            te_p2l(m, "TransformationEstimationPointToPlane",
                   "Class to estimate a transformation for point to plane "
                   "distance.");
    py::detail::bind_default_constructor<
            pipelines::registration::TransformationEstimationPointToPlane>(
            te_p2l);
    py::detail::bind_copy_functions<
            pipelines::registration::TransformationEstimationPointToPlane>(
            te_p2l);
    te_p2l.def("__repr__",
               [](const pipelines::registration::
                          TransformationEstimationPointToPlane &te) {
                   return std::string("TransformationEstimationPointToPlane");
               });

    // open3d.registration.CorrespondenceChecker
    py::class_<pipelines::registration::CorrespondenceChecker,
               PyCorrespondenceChecker<
                       pipelines::registration::CorrespondenceChecker>>
            cc(m, "CorrespondenceChecker",
               "Base class that checks if two (small) point clouds can be "
               "aligned. This class is used in feature based matching "
               "algorithms (such as RANSAC and FastGlobalRegistration) to "
               "prune out outlier correspondences. The virtual function "
               "Check() must be implemented in subclasses.");
    cc.def("Check", &pipelines::registration::CorrespondenceChecker::Check,
           "source"_a, "target"_a, "corres"_a, "transformation"_a,
           "Function to check if two points can be aligned. The two input "
           "point clouds must have exact the same number of points.");
    cc.def_readwrite(
            "require_pointcloud_alignment_",
            &pipelines::registration::CorrespondenceChecker::
                    require_pointcloud_alignment_,
            "Some checkers do not require point clouds to be aligned, e.g., "
            "the edge length checker. Some checkers do, e.g., the distance "
            "checker.");
    docstring::ClassMethodDocInject(
            m, "CorrespondenceChecker", "Check",
            {{"source", "Source point cloud."},
             {"target", "Target point cloud."},
             {"corres",
              "Correspondence set between source and target point cloud."},
             {"transformation", "The estimated transformation (inplace)."}});

    // open3d.registration.CorrespondenceCheckerBasedOnEdgeLength:
    // CorrespondenceChecker
    py::class_<pipelines::registration::CorrespondenceCheckerBasedOnEdgeLength,
               PyCorrespondenceChecker<
                       pipelines::registration::
                               CorrespondenceCheckerBasedOnEdgeLength>,
               pipelines::registration::CorrespondenceChecker>
            cc_el(m, "CorrespondenceCheckerBasedOnEdgeLength",
                  "Check if two point clouds build the polygons with similar "
                  "edge lengths. That is, checks if the lengths of any two "
                  "arbitrary edges (line formed by two vertices) individually "
                  "drawn withinin source point cloud and within the target "
                  "point cloud with correspondences are similar. The only "
                  "parameter similarity_threshold is a number between 0 "
                  "(loose) and 1 (strict)");
    py::detail::bind_copy_functions<
            pipelines::registration::CorrespondenceCheckerBasedOnEdgeLength>(
            cc_el);
    cc_el.def(py::init([](double similarity_threshold) {
                  return new pipelines::registration::
                          CorrespondenceCheckerBasedOnEdgeLength(
                                  similarity_threshold);
              }),
              "similarity_threshold"_a = 0.9)
            .def("__repr__",
                 [](const pipelines::registration::
                            CorrespondenceCheckerBasedOnEdgeLength &c) {
                     return fmt::format(
                             "pipelines::registration::"
                             "CorrespondenceCheckerBasedOnEdgeLength "
                             "with similarity_threshold={:f}",
                             c.similarity_threshold_);
                 })
            .def_readwrite(
                    "similarity_threshold",
                    &pipelines::registration::
                            CorrespondenceCheckerBasedOnEdgeLength::
                                    similarity_threshold_,
                    R"(float value between 0 (loose) and 1 (strict): For the
check to be true,

:math:`||\text{edge}_{\text{source}}|| > \text{similarity_threshold} \times ||\text{edge}_{\text{target}}||` and

:math:`||\text{edge}_{\text{target}}|| > \text{similarity_threshold} \times ||\text{edge}_{\text{source}}||`

must hold true for all edges.)");

    // open3d.registration.CorrespondenceCheckerBasedOnDistance:
    // CorrespondenceChecker
    py::class_<pipelines::registration::CorrespondenceCheckerBasedOnDistance,
               PyCorrespondenceChecker<
                       pipelines::registration::
                               CorrespondenceCheckerBasedOnDistance>,
               pipelines::registration::CorrespondenceChecker>
            cc_d(m, "CorrespondenceCheckerBasedOnDistance",
                 "Class to check if aligned point clouds are close (less than "
                 "specified threshold).");
    py::detail::bind_copy_functions<
            pipelines::registration::CorrespondenceCheckerBasedOnDistance>(
            cc_d);
    cc_d.def(py::init([](double distance_threshold) {
                 return new pipelines::registration::
                         CorrespondenceCheckerBasedOnDistance(
                                 distance_threshold);
             }),
             "distance_threshold"_a)
            .def("__repr__",
                 [](const pipelines::registration::
                            CorrespondenceCheckerBasedOnDistance &c) {
                     return fmt::format(
                             "pipelines::registration::"
                             "CorrespondenceCheckerBasedOnDistance with "
                             "distance_threshold={:f}",
                             c.distance_threshold_);
                 })
            .def_readwrite("distance_threshold",
                           &pipelines::registration::
                                   CorrespondenceCheckerBasedOnDistance::
                                           distance_threshold_,
                           "Distance threshold for the check.");

    // open3d.registration.CorrespondenceCheckerBasedOnNormal:
    // CorrespondenceChecker
    py::class_<
            pipelines::registration::CorrespondenceCheckerBasedOnNormal,
            PyCorrespondenceChecker<pipelines::registration::
                                            CorrespondenceCheckerBasedOnNormal>,
            pipelines::registration::CorrespondenceChecker>
            cc_n(m, "CorrespondenceCheckerBasedOnNormal",
                 "Class to check if two aligned point clouds have similar "
                 "normals. It considers vertex normal affinity of any "
                 "correspondences. It computes dot product of two normal "
                 "vectors. It takes radian value for the threshold.");
    py::detail::bind_copy_functions<
            pipelines::registration::CorrespondenceCheckerBasedOnNormal>(cc_n);
    cc_n.def(py::init([](double normal_angle_threshold) {
                 return new pipelines::registration::
                         CorrespondenceCheckerBasedOnNormal(
                                 normal_angle_threshold);
             }),
             "normal_angle_threshold"_a)
            .def("__repr__",
                 [](const pipelines::registration::
                            CorrespondenceCheckerBasedOnNormal &c) {
                     return fmt::format(
                             "pipelines::registration::"
                             "CorrespondenceCheckerBasedOnNormal with "
                             "normal_threshold={:f}",
                             c.normal_angle_threshold_);
                 })
            .def_readwrite("normal_angle_threshold",
                           &pipelines::registration::
                                   CorrespondenceCheckerBasedOnNormal::
                                           normal_angle_threshold_,
                           "Radian value for angle threshold.");

    // open3d.registration.FastGlobalRegistrationOption:
    py::class_<pipelines::registration::FastGlobalRegistrationOption>
            fgr_option(m, "FastGlobalRegistrationOption",
                       "Options for FastGlobalRegistration.");
    py::detail::bind_copy_functions<
            pipelines::registration::FastGlobalRegistrationOption>(fgr_option);
    fgr_option
            .def(py::init([](double division_factor, bool use_absolute_scale,
                             bool decrease_mu,
                             double maximum_correspondence_distance,
                             int iteration_number, double tuple_scale,
                             int maximum_tuple_count) {
                     return new pipelines::registration::
                             FastGlobalRegistrationOption(
                                     division_factor, use_absolute_scale,
                                     decrease_mu,
                                     maximum_correspondence_distance,
                                     iteration_number, tuple_scale,
                                     maximum_tuple_count);
                 }),
                 "division_factor"_a = 1.4, "use_absolute_scale"_a = false,
                 "decrease_mu"_a = false,
                 "maximum_correspondence_distance"_a = 0.025,
                 "iteration_number"_a = 64, "tuple_scale"_a = 0.95,
                 "maximum_tuple_count"_a = 1000)
            .def_readwrite(
                    "division_factor",
                    &pipelines::registration::FastGlobalRegistrationOption::
                            division_factor_,
                    "float: Division factor used for graduated non-convexity.")
            .def_readwrite(
                    "use_absolute_scale",
                    &pipelines::registration::FastGlobalRegistrationOption::
                            use_absolute_scale_,
                    "bool: Measure distance in absolute scale (1) or in scale "
                    "relative to the diameter of the model (0).")
            .def_readwrite("decrease_mu",
                           &pipelines::registration::
                                   FastGlobalRegistrationOption::decrease_mu_,
                           "bool: Set to ``True`` to decrease scale mu by "
                           "``division_factor`` for graduated non-convexity.")
            .def_readwrite(
                    "maximum_correspondence_distance",
                    &pipelines::registration::FastGlobalRegistrationOption::
                            maximum_correspondence_distance_,
                    "float: Maximum correspondence distance.")
            .def_readwrite(
                    "iteration_number",
                    &pipelines::registration::FastGlobalRegistrationOption::
                            iteration_number_,
                    "int: Maximum number of iterations.")
            .def_readwrite(
                    "tuple_scale",
                    &pipelines::registration::FastGlobalRegistrationOption::
                            tuple_scale_,
                    "float: Similarity measure used for tuples of feature "
                    "points.")
            .def_readwrite(
                    "maximum_tuple_count",
                    &pipelines::registration::FastGlobalRegistrationOption::
                            maximum_tuple_count_,
                    "float: Maximum tuple numbers.")
            .def("__repr__",
                 [](const pipelines::registration::FastGlobalRegistrationOption
                            &c) {
                     return fmt::format(
                             "pipelines::registration::"
                             "FastGlobalRegistrationOption class "
                             "with \ndivision_factor={}"
                             "\nuse_absolute_scale={}"
                             "\ndecrease_mu={}"
                             "\nmaximum_correspondence_distance={}"
                             "\niteration_number={}"
                             "\ntuple_scale={}"
                             "\nmaximum_tuple_count={}",
                             c.division_factor_, c.use_absolute_scale_,
                             c.decrease_mu_, c.maximum_correspondence_distance_,
                             c.iteration_number_, c.tuple_scale_,
                             c.maximum_tuple_count_);
                 });

    // open3d.registration.RegistrationResult
    py::class_<pipelines::registration::RegistrationResult> registration_result(
            m, "RegistrationResult",
            "Class that contains the registration results.");
    py::detail::bind_default_constructor<
            pipelines::registration::RegistrationResult>(registration_result);
    py::detail::bind_copy_functions<
            pipelines::registration::RegistrationResult>(registration_result);
    registration_result
            .def_readwrite("transformation",
                           &pipelines::registration::RegistrationResult::
                                   transformation_,
                           "``4 x 4`` float64 numpy array: The estimated "
                           "transformation matrix.")
            .def_readwrite(
                    "correspondence_set",
                    &pipelines::registration::RegistrationResult::
                            correspondence_set_,
                    "``n x 2`` int numpy array: Correspondence set between "
                    "source and target point cloud.")
            .def_readwrite(
                    "inlier_rmse",
                    &pipelines::registration::RegistrationResult::inlier_rmse_,
                    "float: RMSE of all inlier correspondences. Lower "
                    "is better.")
            .def_readwrite(
                    "fitness",
                    &pipelines::registration::RegistrationResult::fitness_,
                    "float: The overlapping area (# of inlier correspondences "
                    "/ # of points in target). Higher is better.")
            .def("__repr__",
                 [](const pipelines::registration::RegistrationResult &rr) {
                     return fmt::format(
                             "pipelines::registration::RegistrationResult with "
                             "fitness={:e}"
                             ", inlier_rmse={:e}"
                             ", and correspondence_set size of {:d}"
                             "\nAccess transformation to get result.",
                             rr.fitness_, rr.inlier_rmse_,
                             rr.correspondence_set_.size());
                 });
}

// Registration functions have similar arguments, sharing arg docstrings
static const std::unordered_map<std::string, std::string>
        map_shared_argument_docstrings = {
                {"checkers",
                 "Vector of Checker class to check if two point "
                 "clouds can be aligned. One of "
                 "(``pipelines::registration::"
                 "CorrespondenceCheckerBasedOnEdgeLength``, "
                 "``pipelines::registration::"
                 "CorrespondenceCheckerBasedOnDistance``, "
                 "``pipelines::registration::"
                 "CorrespondenceCheckerBasedOnNormal``)"},
                {"corres",
                 "o3d.utility.Vector2iVector that stores indices of "
                 "corresponding point or feature arrays."},
                {"criteria", "Convergence criteria"},
                {"estimation_method",
                 "Estimation method. One of "
                 "(``pipelines::registration::"
                 "TransformationEstimationPointToPoint``, "
                 "``pipelines::registration::"
                 "TransformationEstimationPointToPlane``)"},
                {"init", "Initial transformation estimation"},
                {"lambda_geometric", "lambda_geometric value"},
                {"max_correspondence_distance",
                 "Maximum correspondence points-pair distance."},
                {"option", "Registration option"},
                {"ransac_n", "Fit ransac with ``ransac_n`` correspondences"},
                {"source_feature", "Source point cloud feature."},
                {"source", "The source point cloud."},
                {"target_feature", "Target point cloud feature."},
                {"target", "The target point cloud."},
                {"transformation",
                 "The 4x4 transformation matrix to transform ``source`` to "
                 "``target``"}};

void pybind_registration_methods(py::module &m) {
    m.def("evaluate_registration",
          &pipelines::registration::EvaluateRegistration,
          "Function for evaluating registration between point clouds",
          "source"_a, "target"_a, "max_correspondence_distance"_a,
          "transformation"_a = Eigen::Matrix4d::Identity());
    docstring::FunctionDocInject(m, "evaluate_registration",
                                 map_shared_argument_docstrings);

    m.def("registration_icp", &pipelines::registration::RegistrationICP,
          "Function for ICP registration", "source"_a, "target"_a,
          "max_correspondence_distance"_a,
          "init"_a = Eigen::Matrix4d::Identity(),
          "estimation_method"_a =
                  pipelines::registration::TransformationEstimationPointToPoint(
                          false),
          "criteria"_a = pipelines::registration::ICPConvergenceCriteria());
    docstring::FunctionDocInject(m, "registration_icp",
                                 map_shared_argument_docstrings);

    m.def("registration_colored_icp",
          &pipelines::registration::RegistrationColoredICP,
          "Function for Colored ICP registration", "source"_a, "target"_a,
          "max_correspondence_distance"_a,
          "init"_a = Eigen::Matrix4d::Identity(),
          "criteria"_a = pipelines::registration::ICPConvergenceCriteria(),
          "lambda_geometric"_a = 0.968);
    docstring::FunctionDocInject(m, "registration_colored_icp",
                                 map_shared_argument_docstrings);

    m.def("registration_ransac_based_on_correspondence",
          &pipelines::registration::RegistrationRANSACBasedOnCorrespondence,
          "Function for global RANSAC registration based on a set of "
          "correspondences",
          "source"_a, "target"_a, "corres"_a, "max_correspondence_distance"_a,
          "estimation_method"_a =
                  pipelines::registration::TransformationEstimationPointToPoint(
                          false),
          "ransac_n"_a = 6,
          "criteria"_a = pipelines::registration::RANSACConvergenceCriteria());
    docstring::FunctionDocInject(m,
                                 "registration_ransac_based_on_correspondence",
                                 map_shared_argument_docstrings);

    m.def("registration_ransac_based_on_feature_matching",
          &pipelines::registration::RegistrationRANSACBasedOnFeatureMatching,
          "Function for global RANSAC registration based on feature matching",
          "source"_a, "target"_a, "source_feature"_a, "target_feature"_a,
          "max_correspondence_distance"_a,
          "estimation_method"_a =
                  pipelines::registration::TransformationEstimationPointToPoint(
                          false),
          "ransac_n"_a = 4,
          "checkers"_a = std::vector<std::reference_wrapper<
                  const pipelines::registration::CorrespondenceChecker>>(),
          "criteria"_a = pipelines::registration::RANSACConvergenceCriteria(
                  100000, 100));
    docstring::FunctionDocInject(
            m, "registration_ransac_based_on_feature_matching",
            map_shared_argument_docstrings);

    m.def("registration_fast_based_on_feature_matching",
          &pipelines::registration::FastGlobalRegistration,
          "Function for fast global registration based on feature matching",
          "source"_a, "target"_a, "source_feature"_a, "target_feature"_a,
          "option"_a = pipelines::registration::FastGlobalRegistrationOption());
    docstring::FunctionDocInject(m,
                                 "registration_fast_based_on_feature_matching",
                                 map_shared_argument_docstrings);

    m.def("get_information_matrix_from_point_clouds",
          &pipelines::registration::GetInformationMatrixFromPointClouds,
          "Function for computing information matrix from transformation "
          "matrix",
          "source"_a, "target"_a, "max_correspondence_distance"_a,
          "transformation"_a);
    docstring::FunctionDocInject(m, "get_information_matrix_from_point_clouds",
                                 map_shared_argument_docstrings);
}

void pybind_registration(py::module &m) {
    py::module m_submodule =
            m.def_submodule("registration", "Registration pipeline.");
    pybind_registration_classes(m_submodule);
    pybind_registration_methods(m_submodule);

    pybind_feature(m_submodule);
    pybind_feature_methods(m_submodule);
    pybind_global_optimization(m_submodule);
    pybind_global_optimization_methods(m_submodule);
}

}  // namespace open3d