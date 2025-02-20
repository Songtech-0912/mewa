/****************************************************************************
** Copyright (C) 2020-2021 Mewatools <hugo@mewatools.com>
** SPDX-License-Identifier: MIT License
****************************************************************************/
#include "mxvectorprogram.h"
#include "mxrenderer.h"


MxVectorProgram::MxVectorProgram()
{
}

MxVectorProgram::~MxVectorProgram()
{
    pRenderer->glDeleteProgram( mProgramId );
}


void MxVectorProgram::init( MxRenderer *renderer )
{
    Q_ASSERT( NULL == pRenderer );
    pRenderer = renderer;
}

void MxVectorProgram::initializeGL()
{

    GLuint vshader = pRenderer->glCreateShader(GL_VERTEX_SHADER);
    const char *vsrc1 =
            "attribute  vec2 vertex;\n"
            "attribute  vec2 aUVT;\n"
            "attribute  vec4 color;\n"
            "uniform  mat4 matrix;\n"
            "varying  vec2 fsUVT;\n"
            "varying  vec4 vColor;\n"
            "void main(void)\n"
            "{\n"
            "   fsUVT = aUVT;\n"
            "   vColor = color;\n"
            "   gl_Position = matrix * vec4(vertex.x, vertex.y, 0.0, 1.0);\n"
            "}\n";


    pRenderer->glShaderSource(vshader, 1, &vsrc1, NULL);
    pRenderer->glCompileShader(vshader);
    GLint compiled;
    pRenderer->glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    qDebug("MxVectorProgram vertex shader compiled: %d", compiled);


    GLuint fshader = pRenderer->glCreateShader(GL_FRAGMENT_SHADER);
    const char *fsrc_with_dev =
        #ifdef MX_OPENGL_ES
            "#extension GL_OES_standard_derivatives : enable\n"
            "precision highp float;\n"
        #endif
            "varying  vec2 fsUVT;\n"
            "varying  vec4 vColor;\n"
            "void main()\n"
            "{\n"
            "  float inside = sign(vColor.a - 0.5);\n" // + to draw outside, - to draw inside
            //"  float alphaValue = 1.0;\n"
            "  vec2 dx = dFdx(fsUVT);\n"
            "  vec2 dy = dFdy(fsUVT);\n"
            "  float fx = (2.0*fsUVT.x)*dx.x - dx.y;\n"
            "  float fy = (2.0*fsUVT.x)*dy.x - dy.y;\n"
            "  float sdf = (fsUVT.x*fsUVT.x - fsUVT.y)/sqrt(fx*fx + fy*fy);\n"
            "  float alpha = smoothstep(inside, -inside, sdf);"
            "  gl_FragColor = vColor;\n"
            //"  gl_FragColor.a = min(alphaValue, alpha);"
            "  gl_FragColor.a = alpha;"
            "}\n";

    pRenderer->glShaderSource(fshader, 1, &fsrc_with_dev, NULL);
    pRenderer->glCompileShader(fshader);
    pRenderer->glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE)
    {
        // \TODO use https://stackoverflow.com/questions/22442304/glsl-es-dfdx-dfdy-analog

        qDebug("extension GL_OES_standard_derivatives not supported, falling to aliased shader...\n");
        const char *fsrc =
                "precision highp float;\n"
                "varying  vec2 fsUVT;\n"
                "varying  vec4 vColor;\n"
                "void main()\n"
                "{\n"
                "  float alpha = fsUVT.x * fsUVT.x - fsUVT.y;\n"
                "  alpha = step(0.01, alpha);\n"
                "  gl_FragColor = vec4(vColor.rgb, 1.0-alpha);\n"
                "}\n";
        pRenderer->glShaderSource(fshader, 1, &fsrc, NULL);
        pRenderer->glCompileShader(fshader);
        pRenderer->glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    }


    if (compiled != GL_TRUE)
    {
        GLint infoLogLength;
        pRenderer->glGetShaderiv(fshader, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* errMsg = new char[infoLogLength + 1];
        pRenderer->glGetShaderInfoLog(fshader, infoLogLength, NULL, errMsg);
        qDebug("MxVectorProgram shader compilation error: %s", errMsg);
    }
    else
    {
        qDebug("MxVectorProgram shader compilation completed sucessfully");
    }


    mProgramId = pRenderer->glCreateProgram();
    pRenderer->glAttachShader(mProgramId, vshader);
    pRenderer->glAttachShader(mProgramId, fshader);
    pRenderer->glLinkProgram(mProgramId);
    GLint value = 0;
    pRenderer->glGetProgramiv(mProgramId, GL_LINK_STATUS, &value);
    qDebug("MxVectorProgram shader program linked: %d", value);

    matrixUniform1 = pRenderer->glGetUniformLocation(mProgramId, "matrix");
    mVertexAttrib = pRenderer->glGetAttribLocation(mProgramId, "vertex");
    mUvAttrib = pRenderer->glGetAttribLocation(mProgramId, "aUVT");
    mColorAttrib = pRenderer->glGetAttribLocation(mProgramId, "color");

    pRenderer->checkGLError(__FILE__, __LINE__);

    // \TODO reuse frag shader

    // delete shaders as they are not needed anymore
    pRenderer->glDetachShader(mProgramId, vshader);
    pRenderer->glDetachShader(mProgramId, fshader);
    // glDeleteShader( vshader );
    // glDeleteShader( fshader );

}

void MxVectorProgram::setMatrix( const MxMatrix &matrix )
{
    pRenderer->glUniformMatrix4fv(matrixUniform1, 1, GL_FALSE, matrix.constData());
}

void MxVectorProgram::draw( MxVectorDraw &stream )
{
    stream.pArray->uploadGL(pRenderer);
    enableVao( stream.pArray );
    pRenderer->glDrawArrays( GL_TRIANGLES , 0, stream.pointCount() );
    disableVao();
}

MxShaderProgram::VaoFormat MxVectorProgram::getVaoFormat()
{
    return MxShaderProgram::Float2_UChar4_Float2;
}

MxShaderProgram::VaoFormat MxVectorProgram::vaoFormat()
{
    return MxShaderProgram::Float2_UChar4_Float2;
}

void MxVectorProgram::enableAttributes()
{
    pRenderer->glEnableVertexAttribArray(mVertexAttrib);
    pRenderer->glEnableVertexAttribArray(mUvAttrib);
    pRenderer->glEnableVertexAttribArray(mColorAttrib);

    // \TODO in OpenGL ES2 this does not need to be called always
    uintptr_t offset = 0;
    pRenderer->glVertexAttribPointer(mVertexAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(MxVectorDraw::Vertex), (void *)offset);
    offset += (2*sizeof(GLfloat));
    pRenderer->glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(MxVectorDraw::Vertex), (void *)offset);
    offset += (4*sizeof(GLubyte));
    pRenderer->glVertexAttribPointer(mUvAttrib , 2, GL_FLOAT, GL_FALSE, sizeof(MxVectorDraw::Vertex), (void *)offset);
}

void MxVectorProgram::disableAttributes()
{
    pRenderer->glBindBuffer(GL_ARRAY_BUFFER, 0); // \TODO needed ??
    pRenderer->glDisableVertexAttribArray(mVertexAttrib);
    pRenderer->glDisableVertexAttribArray(mUvAttrib);
    pRenderer->glDisableVertexAttribArray(mColorAttrib);
}

