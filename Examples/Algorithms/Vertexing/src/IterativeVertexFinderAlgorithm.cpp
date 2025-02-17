// This file is part of the Acts project.
//
// Copyright (C) 2019-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Vertexing/IterativeVertexFinderAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/detail/VoidPropagatorComponents.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/IterativeVertexFinder.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "ActsExamples/EventData/ProtoVertex.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <chrono>
#include <ostream>
#include <stdexcept>
#include <system_error>

#include "VertexingHelpers.hpp"

ActsExamples::IterativeVertexFinderAlgorithm::IterativeVertexFinderAlgorithm(
    const Config& config, Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("IterativeVertexFinder", level), m_cfg(config) {
  if (m_cfg.inputTrackParameters.empty()) {
    throw std::invalid_argument("Missing input track parameter collection");
  }
  if (m_cfg.outputProtoVertices.empty()) {
    throw std::invalid_argument("Missing output proto vertices collection");
  }
  if (m_cfg.outputVertices.empty()) {
    throw std::invalid_argument("Missing output vertices collection");
  }

  m_inputTrackParameters.initialize(m_cfg.inputTrackParameters);
  m_outputProtoVertices.initialize(m_cfg.outputProtoVertices);
  m_outputVertices.initialize(m_cfg.outputVertices);
}

ActsExamples::ProcessCode ActsExamples::IterativeVertexFinderAlgorithm::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // retrieve input tracks and convert into the expected format

  const auto& inputTrackParameters = m_inputTrackParameters(ctx);
  // TODO change this from pointers to tracks parameters to actual tracks
  auto inputTrackPointers =
      makeTrackParametersPointerContainer(inputTrackParameters);

  if (inputTrackParameters.size() != inputTrackPointers.size()) {
    ACTS_ERROR("Input track containers do not align: "
               << inputTrackParameters.size()
               << " != " << inputTrackPointers.size());
  }

  for (const auto trk : inputTrackPointers) {
    if (trk->covariance() && trk->covariance()->determinant() <= 0) {
      // actually we should consider this as an error but I do not want the CI
      // to fail
      ACTS_WARNING("input track " << *trk << " has det(cov) = "
                                  << trk->covariance()->determinant());
    }
  }

  // Set up EigenStepper
  Acts::EigenStepper<> stepper(m_cfg.bField);

  // Set up propagator with void navigator
  auto propagator =
      std::make_shared<Propagator>(stepper, Acts::detail::VoidNavigator{},
                                   logger().cloneWithSuffix("Propagator"));
  // Setup the vertex fitter
  Fitter::Config vertexFitterCfg;
  Fitter vertexFitter(vertexFitterCfg,
                      logger().cloneWithSuffix("FullBilloirVertexFitter"));
  // Setup the track linearizer
  Linearizer::Config linearizerCfg(m_cfg.bField, propagator);
  Linearizer linearizer(linearizerCfg,
                        logger().cloneWithSuffix("HelicalTrackLinearizer"));
  // Setup the seed finder
  IPEstimator::Config ipEstCfg(m_cfg.bField, propagator);
  IPEstimator ipEst(ipEstCfg, logger().cloneWithSuffix("ImpactPointEstimator"));
  Seeder seeder;
  // Set up the actual vertex finder
  Finder::Config finderCfg(std::move(vertexFitter), std::move(linearizer),
                           std::move(seeder), ipEst);
  finderCfg.maxVertices = 200;
  finderCfg.reassignTracksAfterFirstFit = false;
  Finder finder(std::move(finderCfg), logger().clone());
  Finder::State state(*m_cfg.bField, ctx.magFieldContext);
  Options finderOpts(ctx.geoContext, ctx.magFieldContext);

  // find vertices
  auto result = finder.find(inputTrackPointers, finderOpts, state);

  VertexCollection vertices;
  if (result.ok()) {
    vertices = std::move(result.value());
  } else {
    ACTS_ERROR("Error in vertex finder: " << result.error().message());
  }

  // show some debug output
  ACTS_INFO("Found " << vertices.size() << " vertices in event");
  for (const auto& vtx : vertices) {
    ACTS_INFO("Found vertex at " << vtx.fullPosition().transpose() << " with "
                                 << vtx.tracks().size() << " tracks.");
  }

  // store proto vertices extracted from the found vertices
  m_outputProtoVertices(ctx, makeProtoVertices(inputTrackPointers, vertices));

  // store found vertices
  m_outputVertices(ctx, std::move(vertices));

  return ActsExamples::ProcessCode::SUCCESS;
}
