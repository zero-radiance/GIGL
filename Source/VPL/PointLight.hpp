#pragma once

#include "PointLight.h"

template <class T>
const glm::vec3& PointLight<T>::wPos() const {
    return static_cast<const T*>(this)->getWPos();
}

template <class T>
const glm::vec3& PointLight<T>::intensity() const {
    return static_cast<const T*>(this)->getIntensity();
}

template <class T>
const void PointLight<T>::setWPos(const glm::vec3& w_pos) {
    static_cast<T*>(this)->m_w_pos = w_pos;
}

template <class T>
const void PointLight<T>::setIntensity(const glm::vec3& intensity) {
    static_cast<T*>(this)->m_intensity = intensity;
}
