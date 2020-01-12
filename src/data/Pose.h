#pragma once
#ifndef KJY_ANIM_ENGINE_COMPONENTS_POSE_HPP_
#define KJY_ANIM_ENGINE_COMPONENTS_POSE_HPP_
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

class Pose{
 public:
	Pose() {}
	Pose(const glm::vec3& start_location) : translation(start_location) {_update_matrix_cache();}
	Pose(const glm::vec3& start_location, const glm::quat& start_orientation) : translation(start_location), orientation(start_orientation) {_update_matrix_cache();}
	Pose(const glm::vec3& start_location, const glm::quat& start_orientation, const glm::vec3& start_scale) : translation(start_location), orientation(start_orientation), scale(start_scale) {_update_matrix_cache();}
	Pose(const glm::mat4& pre_transform) : preaffine(pre_transform) {_update_matrix_cache();}
	Pose(const glm::mat4& pre_transform, const glm::mat4& post_transform): preaffine(pre_transform), postaffine(post_transform) {_update_matrix_cache();}

	const glm::mat4& getMatrix();
	inline glm::vec3 getCenterOfSpace() const {return(glm::vec3(getMatrix() * glm::vec4(0.0,0.0,0.0,1.0)));}
	inline glm::vec3 getCenterOfSpace() {return(glm::vec3(getMatrix() * glm::vec4(0.0,0.0,0.0,1.0)));}
	inline glm::vec3 getOrientationAsVectorX() const {return(glm::normalize(orientation * glm::vec3(1.0, 0.0, 0.0)));}
	inline glm::vec3 getOrientationAsVectorY() const {return(glm::normalize(orientation * glm::vec3(0.0, 1.0, 1.0)));}
	inline glm::vec3 getOrientationAsVectorZ() const {return(glm::normalize(orientation * glm::vec3(0.0, 0.0, 1.0)));}

	/* Copies contents of other into this pose object. Typically you would
		* want to just use assignment instead, but this is usefull for any
		* classes which inherit from pose and want to be able to update
		* themselves based on a plain pose object. */
    void copyFrom(const Pose& other);

	inline const glm::vec3& getTranslation() const {return(translation);}
	inline const glm::quat& getOrientation() const {return(orientation);}
	inline const glm::vec3& getScale() const {return(scale);}
	inline const glm::mat4& getPreTransformMatrix() const {return(preaffine);}
	inline const glm::mat4& getPostTransformMatrix() const {return(postaffine);}

	/* Returns a new `Pose` that has had all elements which translate the
		* represented space eliminated except for a translation leaving it at
		* `origin`. The post/pre-affine matrices will have their final column
		* overwritten with the identity .
	 */
    inline Pose getCenteredAndReduced(const glm::vec3& origin = glm::vec3(0.0)) const{
		Pose newpose = *this;
		newpose.translation = origin;
		newpose.postaffine[3] = newpose.preaffine[3] = glm::vec4(0.0, 0.0, 0.0, 1.0);
		newpose._update_matrix_cache();
		return(newpose);
	}

	static Pose mix(const Pose& p1, const Pose& p2, float t);

	/* See static getReducedAndReadable. Applies that function to the current pose. */
	Pose getReducedAndReadable() const; 
	
	/* Attempts to take existing pose and extract
	* translation/scale/orientation transformations from the pre and post
	* affine matrices. These transformations are then merged with existing
	* translation/scale/orientation in order to produce a new `Pose` that
	* has a minimal amount of transformation occuring in the post and pre
	* affine matrices, and is more readable */
	static Pose getReducedAndReadable(const glm::mat4& matrix);


	inline void setTranslation(const glm::vec3& new_translation) {_dirty_matrix = true; translation = new_translation;}
	inline void setOrientation(const glm::quat& new_orientation) {_dirty_matrix = true; orientation = new_orientation;}
	inline void setScale(const glm::vec3& new_scale) {_dirty_matrix = true; scale = new_scale;}
	inline void setPostTransformMatrix(const glm::mat4& post_transform){_dirty_matrix = true; postaffine = post_transform;}
	inline void setPreTransformaMatrix(const glm::mat4& pre_transform){_dirty_matrix = true; preaffine = pre_transform;}
	inline void appendTransformToEnd(const glm::mat4& transform) {_dirty_matrix = true; postaffine *= transform;}
	inline void appendTransformToStart(const glm::mat4& transform) {_dirty_matrix = true; preaffine = transform * preaffine;}

	Pose& operator+=(const glm::vec3& rhs);
	Pose& operator-=(const glm::vec3& rhs);
	Pose& operator*=(const glm::vec3& rhs);
	Pose& operator/=(const glm::vec3& rhs);
	Pose& operator*=(const Pose& rhs);
	Pose& operator*=(const glm::mat4& rhs);

	void print() const;
	std::string asString() const;

	operator std::string() const {
		return("{Pose:\n   center @ <" + glm::to_string(getCenterOfSpace()) + "> orientation: <" + glm::to_string(orientation) + "> scale: <" + glm::to_string(scale) + "> \n   " + glm::to_string(getMatrix()) + "\n}");
	}
	operator std::string() {
		return("{Pose:\n   center @ <" + glm::to_string(getCenterOfSpace()) + "> orientation: <" + glm::to_string(orientation) + "> scale: <" + glm::to_string(scale) + "> \n   " + glm::to_string(getMatrix()) + "\n}");
	}
	operator glm::mat4() const {return(getMatrix());}
	operator glm::mat4() {return(getMatrix());}

	inline glm::mat4 getMatrix() const{
		if(_dirty_matrix){
			return(postaffine * glm::translate(glm::mat4(1.0), translation) * glm::mat4_cast(orientation) * glm::scale(glm::mat4(1.0), scale) * preaffine);
		}
		return(_cache_matrix);
	}

	inline void updateMatrixCache(){_update_matrix_cache();}

 protected:
	glm::vec3 translation = glm::vec3(0.0);
	glm::quat orientation = glm::quat(1.0, 0.0, 0.0, 0.0);
	glm::vec3 scale = glm::vec3(1.0);
	glm::mat4 preaffine = glm::mat4(1.0);
	glm::mat4 postaffine = glm::mat4(1.0);

	inline void _update_matrix_cache(){
		_cache_matrix = postaffine * glm::translate(glm::mat4(1.0), translation) * glm::mat4_cast(orientation) * glm::scale(glm::mat4(1.0), scale) * preaffine;
		_dirty_matrix = false;
	}

 private:
	bool _dirty_matrix = true;
	glm::mat4 _cache_matrix; 

 public:
	friend Pose operator+(Pose lhs, const glm::vec3& rhs){
		lhs.translation += rhs;
		lhs._update_matrix_cache();
		return(lhs);
	}
	friend Pose operator-(Pose lhs, const glm::vec3& rhs){
		lhs.translation -= rhs;
		lhs._update_matrix_cache();
		return(lhs);
	}
	friend Pose operator*(Pose lhs, const glm::vec3& rhs){
		lhs.translation *= rhs;
		lhs._update_matrix_cache();
		return(lhs);
	}
	friend Pose operator/(Pose lhs, const glm::vec3& rhs){
		lhs.translation /= rhs;
		lhs._update_matrix_cache();
		return(lhs);
	}
	friend Pose operator*(Pose lhs, const Pose& rhs){
		lhs.preaffine = lhs.preaffine * rhs.getMatrix();
		lhs._update_matrix_cache();
		return(lhs);
	}
	friend Pose operator*(Pose lhs, const glm::mat4& rhs){
		lhs.preaffine = lhs.preaffine * rhs;
		lhs._update_matrix_cache();
		return(lhs);
	}
};

#endif
