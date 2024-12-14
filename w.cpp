// Copyright (c) 2019 Windy Darian. MIT License.
// Custom script library for things.

#include "w.h"

#include "core/variant/dictionary.h"

using w_tag_t = int32_t;
const w_tag_t k_tag_null = 0;
const int k_tag_segment_bits = 8;
const int k_tag_max_segments = 4;

class w_impl_t
{
	/* Internal data of module. Currently mainly used for a tag system (inspired from Unreal's gameplay tag system). */
public:
	Dictionary tag_dictionary;  // String to array [int tag_id, int next available segment sub id from 1]
	int next_root_tag_id = 1;  // Initialize to 1 so 0 is a "null" option
	/*
	** Tag ID: a.b.c.d has ddccbbaa order! children always take most significant bits.
	*/

	void define_tag(const String& tag_name);
	w_tag_t get_tag(const String& tag_name);
	bool match_tag(w_tag_t a, w_tag_t b);
};

_W* _W::singleton = nullptr;

void w_impl_t::define_tag(const String& tag_name)
{
	int split_location = -1;

	Variant arr_parent = Variant();
	for (int segment = 0; ; ++segment)
	{
		ERR_FAIL_COND_MSG(segment >= k_tag_max_segments, "Tag has too many segment (.) s!");

		split_location = tag_name.find_char('.', split_location + 1);
		String current_parent_tag_name = tag_name.substr(0, split_location);
		//Variant current_parent_tag_name_variant = {current_parent_tag_name};
		Variant* p_value = tag_dictionary.getptr(current_parent_tag_name);
		if (p_value)
		{
			if (split_location == -1)
			{
				ERR_PRINT((String("tag already exists!") + tag_name));
				return;
			}
			ERR_FAIL_COND_MSG(p_value->get_type() != Variant::Type::ARRAY, "Invalid type from tag dictionary. Expect array with [int tag_id, int sub_tag_count]");

			arr_parent = *p_value;
		}
		else
		{
			int next_segment_id;
			int parent_id;
			if (arr_parent)
			{
				Array arr_parent_arr = arr_parent.operator Array();
				parent_id = arr_parent_arr[0];
				next_segment_id = arr_parent_arr[1];
			}
			else
			{
				next_segment_id = next_root_tag_id;
				parent_id = 0;
			}
			ERR_FAIL_COND_MSG(next_segment_id >= (1 << k_tag_segment_bits), "Parent segment has too many sub tags!");

			Array new_entry;
			new_entry.resize(2);
			int new_entry_id = parent_id | (next_segment_id << (segment * k_tag_segment_bits));
			int inital_segment_id = 1;
			new_entry[0] = Variant(new_entry_id);
			new_entry[1] = Variant(inital_segment_id);

			tag_dictionary[current_parent_tag_name] = new_entry;

			if (arr_parent)
			{
				Array parent_array = arr_parent.operator Array();
				parent_array[1] = Variant(parent_array[1].operator signed int() + 1);
			}
			else
			{
				// No parent. We must be root.
				++next_root_tag_id;
			}
			arr_parent = new_entry;
		}

		if (split_location < 0)
		{
			break;
		}
	}
}
void _W::define_tag(const String& tag_name)
{
	impl->define_tag(tag_name);
}

w_tag_t w_impl_t::get_tag(const String& tag_name)
{
	Variant* p_value = tag_dictionary.getptr(tag_name);
	if (!p_value)
	{
		ERR_PRINT("Unregistered tag: " + tag_name);
		return k_tag_null;
	}
	return w_tag_t(p_value->operator Array()[0]);
}
int _W::get_tag(const String& tag_name)
{
	return impl->get_tag(tag_name);
}

bool w_impl_t::match_tag(w_tag_t a, w_tag_t b)
{
	// returns true if a is same or derived from b
	// a "foo.bar" matches b "foo"
	// a "foo" does NOT match b "foo.bar"

	ERR_FAIL_COND_V_MSG(a == k_tag_null, false, "Invalid tag a");
	ERR_FAIL_COND_V_MSG(b == k_tag_null, false, "Invalid tag b");

	uint32_t mask = (1 << k_tag_segment_bits) - 1;
	for (int i = 0; i < k_tag_max_segments; ++i)
	{
		uint32_t b_current_segment = (b >> i * k_tag_segment_bits) & mask;
		if (!b_current_segment)
		{
			break;
		}

		uint32_t a_current_segment = (a >> i * k_tag_segment_bits) & mask;
		if (a_current_segment == b_current_segment)
		{
			continue;
		}
		else
		{
			return false;
		}

	}
	return true;
}
bool _W::match_tag(int a, int b)
{
	return impl->match_tag(a, b);
}

static void _spatial_set_rotation_quat(Node3D *spatial, const Quaternion &rotation)
{
	ERR_FAIL_NULL(spatial);
	Transform3D transform = spatial->get_transform();
	transform.set_basis(Basis(rotation));
	spatial->set_transform(transform);
}
void _W::spatial_set_rotation_quat(Node *spatial, const Quaternion &rotation) const   // Note: Node3D doesn't have conversion to Variant...
{
	_spatial_set_rotation_quat(Object::cast_to<Node3D>(spatial), rotation);
}

static void _spatial_set_rotation_quat_keep_scale(Node3D *spatial, const Quaternion &rotation)
{
	ERR_FAIL_NULL(spatial);
	Transform3D transform = spatial->get_transform();
	Vector3 original_scale(transform.basis.get_scale());
	transform.set_basis(Basis(rotation, original_scale));
	spatial->set_transform(transform);
}
void _W::spatial_set_rotation_quat_keep_scale(Node* spatial, const Quaternion& rotation) const
{
	_spatial_set_rotation_quat_keep_scale(Object::cast_to<Node3D>(spatial), rotation);
}

Quaternion _W::quat(Vector3 forward, Vector3 up) {
	Vector3 y = up;
	Vector3 z = -forward;
	Vector3 x = y.cross(z);
	y = z.cross(x);  // Make sure orthonormal - up may change.
	if (x.is_zero_approx() || y.is_zero_approx() || z.is_zero_approx()) {
		// TODO: warn
		return Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
	}
	// TODO: use a more direct math without Basis?
	Basis basis{};
	// Note: we want to set columns; constructor sets rows instead.
	basis.set_column(0, x.normalized());
	basis.set_column(1, y.normalized());
	basis.set_column(2, z.normalized());
	return basis.get_quaternion();
}

Quaternion _W::quat_from_up(Vector3 up, Vector3 forward) {
	Vector3 y = up;
	Vector3 z = -forward;
	Vector3 x = y.cross(z);
	z = x.cross(y);  // Make sure orthonormal - forward may change.
	if (x.is_zero_approx() || y.is_zero_approx() || z.is_zero_approx()) {
		// TODO: warn
		return Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
	}
	// TODO: use a more direct math without Basis?
	Basis basis{};
	// Note: we want to set columns; constructor sets rows instead.
	basis.set_column(0, x.normalized());
	basis.set_column(1, y.normalized());
	basis.set_column(2, z.normalized());
	return basis.get_quaternion();
}

Quaternion _W::quat_align_forward(Vector3 forward, Quaternion quat) {
	if (forward.is_zero_approx()) {
		return quat;
	}
	Vector3 y = quat.xform(Vector3{ 0.0f, 1.0f, 0.0f });
	Vector3 z = -forward;
	Vector3 x = y.cross(z);
	y = z.cross(x);
	if (x.is_zero_approx() || y.is_zero_approx() || z.is_zero_approx()) {
		x = quat.xform(Vector3{ 1.0f, 0.0f, 0.0f });
		z = -forward;
		y = z.cross(x);
		x = y.cross(z);
	}
	Basis basis{};
	basis.set_column(0, x.normalized());
	basis.set_column(1, y.normalized());
	basis.set_column(2, z.normalized());
	return basis.get_quaternion();
}

static Quaternion _spatial_get_rotation_quat(const Node3D *spatial)
{
	ERR_FAIL_NULL_V(spatial, Quaternion());
	return spatial->get_transform().basis.get_rotation_quaternion();
}
Quaternion _W::spatial_get_rotation_quat(const Node *spatial) const
{
	return _spatial_get_rotation_quat(Object::cast_to<Node3D>(spatial));
}

void _W::_bind_methods() {
	ClassDB::bind_method(D_METHOD("spatial_set_rotation_quat", "spatial", "quaternion"), &_W::spatial_set_rotation_quat);
	ClassDB::bind_method(D_METHOD("spatial_set_rotation_quat_keep_scale", "spatial", "quaternion"), &_W::spatial_set_rotation_quat_keep_scale);
	ClassDB::bind_method(D_METHOD("spatial_get_rotation_quat", "spatial"), &_W::spatial_get_rotation_quat);

	ClassDB::bind_method(D_METHOD("quat", "forward", "up"), &_W::quat);
	ClassDB::bind_method(D_METHOD("quat_from_up", "up", "forward"), &_W::quat_from_up);
	ClassDB::bind_method(D_METHOD("quat_align_forward", "forward", "quat"), &_W::quat_align_forward);

	ClassDB::bind_method(D_METHOD("define_tag", "tag_name"), &_W::define_tag);
	ClassDB::bind_method(D_METHOD("get_tag", "tag_name"), &_W::get_tag);
	ClassDB::bind_method(D_METHOD("match_tag", "a", "b"), &_W::match_tag);
}

_W::_W()
{
	_W::singleton = this;
	impl = memnew(w_impl_t);
}

_W::~_W()
{
	memdelete(impl);
	impl = nullptr;
}
