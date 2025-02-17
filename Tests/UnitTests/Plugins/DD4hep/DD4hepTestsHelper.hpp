// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

#include <string>

#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Objects.h>
#include <XML/Utilities.h>

using namespace dd4hep;

namespace Acts {
class Surface;
}  // namespace Acts

namespace DD4hepTestsHelper {

/// @brief helper to ensure that an extension is set,
/// copied from the ODD detector code
///
/// @tparam T the type of the extension
/// @param elt the detector element
/// @return the extracted/created extennsion
template <typename T>
T& ensureExtension(dd4hep::DetElement& elt) {
  T* ext = elt.extension<T>(false);
  if (ext == nullptr) {
    ext = new T();
  }
  elt.addExtension<T>(ext);
  return *ext;
}

/// Helper method to create a Transform3D from an xml detector
/// component
///
/// @param x_det_comp the xml detector component
///
/// @return a Transform3D (DD4hep type, aka ROOT::Math type)
Transform3D createTransform(const xml_comp_t& x_det_comp);

/// Helper method to convert an ACTS transform into XML
///
/// @param tf the transform in ACTS format
/// @param axes the identification which axes are building the local frame
///
/// @return a string representing the XML entry
std::string transformToXML(const Acts::Transform3& tf,
                           const std::array<int, 2u>& axes = {0, 1});

/// @brief  Helper method to convert a Surface into XML
///
/// @param gctx the geometry context of this call
/// @param surface the surface from ACTS to be written
/// @param ref the reference transform
///
/// @return a string representing the XML entry
std::string surfaceToXML(const Acts::GeometryContext& gctx,
                         const Acts::Surface& surface,
                         const Acts::Transform3& ref);

}  // namespace DD4hepTestsHelper
