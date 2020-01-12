#include "Pose.h"

const glm::mat4& Pose::getMatrix(){
    if(_dirty_matrix){
        _update_matrix_cache();
    }
    return(_cache_matrix);
}

void Pose::copyFrom(const Pose& other){
    // Just use assignment
    *this = other;
}

Pose Pose::mix(const Pose& p1, const Pose& p2, float t){
    Pose mixture;
    mixture.setTranslation(glm::mix(p1.translation, p2.translation, t));
    mixture.setOrientation(glm::slerp(p1.orientation, p2.orientation, t));
    mixture.setScale(glm::mix(p1.scale, p2.scale, t));
    return(mixture);
}

Pose Pose::getReducedAndReadable() const{
    return(getReducedAndReadable(glm::translate(glm::mat4(1.0), translation) * glm::mat4_cast(orientation) * glm::scale(glm::mat4(1.0), scale) * preaffine));
}

Pose Pose::getReducedAndReadable(const glm::mat4& matrix){
    using namespace glm;
    mat4 premultiply_before_post = matrix;
    vec3 pre_origin = premultiply_before_post * vec4(0.0, 0.0, 0.0, 1.0);

    mat4 elim_translation = glm::translate(premultiply_before_post, -pre_origin);
    quat pre_rotation = quat_cast(elim_translation);
    mat4 elim_rotation = inverse(mat4_cast(pre_rotation)) * elim_translation;

    vec3 shear_check_1 = elim_rotation * vec4(1.0);
    vec3 shear_check_2 = (elim_rotation * vec4(2.0)) * 0.5f;
    vec3 pre_scale = vec3(1.0);
    mat4 elim_scale = elim_rotation;
    if(all(epsilonEqual(shear_check_1, shear_check_2, 1e-7f))){
        pre_scale = shear_check_1;
        elim_scale = glm::scale(elim_scale, 1.0f/pre_scale);
    }

    Pose reduced(elim_scale);
    reduced.setTranslation(pre_origin);
    reduced.setOrientation(pre_rotation);
    reduced.setScale(pre_scale);

    return(reduced);
}

void Pose::print() const{
    std::cout << std::string(*this) << std::endl;
}

std::string Pose::asString() const{
    return(std::string(*this));
}

Pose& Pose::operator+=(const glm::vec3& rhs){
    this->translation += rhs;
    this->_update_matrix_cache();
    return(*this);
}
Pose& Pose::operator-=(const glm::vec3& rhs){
    this->translation -= rhs;
    this->_update_matrix_cache();
    return(*this);
}
Pose& Pose::operator*=(const glm::vec3& rhs){
    this->translation *= rhs;
    this->_update_matrix_cache();
    return(*this);
}
Pose& Pose::operator/=(const glm::vec3& rhs){
    this->translation /= rhs;
    this->_update_matrix_cache();
    return(*this);
}
Pose& Pose::operator*=(const Pose& rhs){
    this->preaffine = this->preaffine * rhs.getMatrix();
    this->_update_matrix_cache();
    return(*this);
}
Pose& Pose::operator*=(const glm::mat4& rhs){
    this->preaffine = this->preaffine * rhs;
    this->_update_matrix_cache();
    return(*this);
}
