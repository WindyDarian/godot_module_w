// Copyright (c) 2019 Windy Darian. MIT License.
// Custom script library for things.

#pragma once

#include "core/object.h"
#include "core/math/quat.h"
#include "scene/3d/node_3d.h"

class w_impl_t;

class _W : public Object {
	GDCLASS(_W, Object);
	static _W* singleton;

protected:
	static void _bind_methods();

public:
	/** Get local rotation of a spatial node as a quaternion. */
	Quat spatial_get_rotation_quat(const Node* spatial) const;
	/** Set local rotation of a spatial node from a quaternion. This will reset local scale to one. */
	void spatial_set_rotation_quat(Node* spatial, const Quat& rotation) const;
	/** Set local rotation of a spatial node from a quaternion. This will keep local scale. */
	void spatial_set_rotation_quat_keep_scale(Node* spatial, const Quat& rotation) const;

	/** Register a new tag. */
	void define_tag(const String& tag_name);
	/** Get tag as int from name. */
	int get_tag(const String& tag_name);
	/**
	** Test if tag a match tag b.
	** returns true if a is same or derived from b
	** a "foo.bar" matches b "foo"
	** a "foo" does NOT match b "foo.bar"
	*/
	bool match_tag(int a, int b);

	static _W* get_singleton() {return singleton;};
	_W();
	virtual ~_W();

private:
	w_impl_t* impl;
};
