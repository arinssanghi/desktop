// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSScale_h
#define CSSScale_h

#include "base/macros.h"
#include "core/css/cssom/CSSTransformComponent.h"
#include "core/css/cssom/CSSUnitValue.h"
#include "core/geometry/DOMMatrix.h"

namespace blink {

class CSSNumericValue;
class DOMMatrix;

// Represents a scale value in a CSSTransformValue used for properties like
// "transform".
// See CSSScale.idl for more information about this class.
class CORE_EXPORT CSSScale final : public CSSTransformComponent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Constructors defined in the IDL.
  static CSSScale* Create(const CSSNumberish&,
                          const CSSNumberish&,
                          ExceptionState&);
  static CSSScale* Create(const CSSNumberish&,
                          const CSSNumberish&,
                          const CSSNumberish&,
                          ExceptionState&);

  // Blink-internal ways of creating CSSScales.
  static CSSScale* Create(CSSNumericValue* x, CSSNumericValue* y) {
    return new CSSScale(x, y, CSSUnitValue::Create(1), true /* is2D */);
  }
  static CSSScale* Create(CSSNumericValue* x,
                          CSSNumericValue* y,
                          CSSNumericValue* z) {
    return new CSSScale(x, y, z, false /* is2D */);
  }
  static CSSScale* FromCSSValue(const CSSFunctionValue&);

  // Getters and setters for attributes defined in the IDL.
  void x(CSSNumberish& x) { x.SetCSSNumericValue(x_); }
  void y(CSSNumberish& y) { y.SetCSSNumericValue(y_); }
  void z(CSSNumberish& z) { z.SetCSSNumericValue(z_); }
  void setX(const CSSNumberish&, ExceptionState&);
  void setY(const CSSNumberish&, ExceptionState&);
  void setZ(const CSSNumberish&, ExceptionState&);

  DOMMatrix* toMatrix(ExceptionState&) const final;

  // Internal methods - from CSSTransformComponent.
  TransformComponentType GetType() const final { return kScaleType; }
  const CSSFunctionValue* ToCSSValue() const final;

  virtual void Trace(blink::Visitor* visitor) {
    visitor->Trace(x_);
    visitor->Trace(y_);
    visitor->Trace(z_);
    CSSTransformComponent::Trace(visitor);
  }

 private:
  CSSScale(CSSNumericValue* x,
           CSSNumericValue* y,
           CSSNumericValue* z,
           bool is2D);

  Member<CSSNumericValue> x_;
  Member<CSSNumericValue> y_;
  Member<CSSNumericValue> z_;

  DISALLOW_COPY_AND_ASSIGN(CSSScale);
};

}  // namespace blink

#endif
