/****************************************************************************
** Copyright (C) 2020-2021 Mewatools <hugo@mewatools.com>
** SPDX-License-Identifier: MIT License
****************************************************************************/
#ifndef MXCOLORWHEELPROGRAM_H
#define MXCOLORWHEELPROGRAM_H

#include "mxdebug.h"
#include "mxopengl.h"
#include "mxshaderprogram.h"
#include "mxmatrix.h"
#include "mxrect.h"


class MxRenderer;


class MxColorWheelProgram : public MxShaderProgram
{
public:

    static MxVector3F colorAt(const MxRectF &rect, const MxVector2F &coord );

    MxColorWheelProgram();
    virtual ~MxColorWheelProgram();

    // static ColorWheelEffect* instanceGL();
    void init( MxRenderer *renderer );
    void initializeGL();

    void setMatrix( const MxMatrix &matrix );

    void setBackgroundColor( float r, float g, float b );

    /*! Used to smooth the border of the circle.
     A tipical value would be 1 / wheelSize. Smaller values cause a bigger blur around. */
    void setSmoothEdge( float threshold );

    virtual VaoFormat vaoFormat() { return Float_4; }
    void draw(const MxRectF &rect, MxRenderer &renderer );

    virtual void enable();
    virtual void disableVao();
    virtual void enableAttributes();
    virtual void disableAttributes();


private:

    enum Update
    {
        UpdateMatrix = 0x00000001,
        BackgroundColor = 0x00000002,
        UpdateSmoothness = 0x00000004,
        UpdateAll = 0x7FFFFFFF
    };

    GLuint pVaoObject;
    MxMatrix pModelview;
    MxVector3F pBackgroundColor;
    float pSmoothEdge;
    int pUpdates;

    // gl
    GLint pVertexAttrib;
    GLint pMatrixUniform;
    GLint pColorUniform;
    GLint pSmoothnessUniform;


};

#endif
