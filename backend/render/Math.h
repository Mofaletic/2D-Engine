#pragma once

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vector4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

class Matrix4 {
public:
    Matrix4();

    const float* Data() const;
    float* Data();

    static Matrix4 Identity();
    static Matrix4 Orthographic(float left, float right, float bottom, float top, float nearClip, float farClip);
    static Matrix4 Translation(float x, float y, float z);
    static Matrix4 Scale(float x, float y, float z);
    static Matrix4 RotationX(float radians);
    static Matrix4 RotationY(float radians);
    static Matrix4 RotationZ(float radians);

    Matrix4 operator*(const Matrix4& other) const;

private:
    float m_Elements[16];
};

struct Transform {
    Vector3 Translation{ 0.0f, 0.0f, 0.0f };
    Vector3 Rotation{ 0.0f, 0.0f, 0.0f };
    Vector3 Scale{ 1.0f, 1.0f, 1.0f };

    Matrix4 ToMatrix() const {
        return Matrix4::Translation(Translation.x, Translation.y, Translation.z)
            * Matrix4::RotationX(Rotation.x)
            * Matrix4::RotationY(Rotation.y)
            * Matrix4::RotationZ(Rotation.z)
            * Matrix4::Scale(Scale.x, Scale.y, Scale.z);
    }
};
