/**
 * @file include/halo/models/animation_view.hpp
 * Sampling and blending of model animation frames (base, overlay and replacement animations).
 */
#pragma once

#include "halo/models/models_types.hpp"

namespace halo::models {

/**
 * Non-owning view of one ModelAnimationsAnimation record of an animation graph tag. It reads the animation's
 * compressed or uncompressed frame data and never owns it.
 */
class animation_view {
public:
    explicit animation_view(ModelAnimationsAnimation *p) : self(p) {}

    /**
     * Returns the address of the frame's data: the compressed header base when the animation is compressed and compressed
     * decoding is enabled, otherwise the byte offset of the requested uncompressed frame within the frame data.
     *
     * @address 0x4d4810
     */
    void * get_frame_data(int16_t frame);

    /**
     * Sums the animation's per-frame x displacement records and writes the displacement up to the key frame and the total
     * displacement.
     *
     * @address 0x4d4850
     */
    void get_frame_info_distance(float *dx_to_key_frame, float *dx_total);

    /**
     * Samples one animation frame into an array of per-node orientations. It starts from the model's bind pose unless the
     * animation is a base animation whose node list matches the model, then reads each node's rotation, translation and
     * scale from the compressed codec, the uncompressed stream or the defaults.
     *
     * @address 0x4d4a80
     */
    void get_frame_orientations(GBXModel *model, int16_t frame, real_orientation *out_orientations);

    /**
     * Evaluates one node's compressed rotation curve at a possibly fractional frame: finds the two bracketing keyframes,
     * using the node default as an implicit keyframe before the first and after the last, and slerps between them.
     *
     * @address 0x4d6b60
     */
    void node_get_rotation(real frame, int16_t rotation_index, int16_t node, real_quaternion *out);

    /**
     * Evaluates one node's compressed translation curve at a possibly fractional frame with the same bracketing scheme as
     * the rotation curve, interpolating linearly.
     *
     * @address 0x4d6cf0
     */
    void node_get_translation(real frame, int16_t translation_index, int16_t node, real_point3d *out);

    /**
     * Evaluates one node's compressed scale curve at a possibly fractional frame. The scale index, not a node index,
     * selects the default value on both sides of the keyframes.
     *
     * @address 0x4d6e80
     */
    void node_get_scale(int16_t scale_index, real frame, real *out);

    /**
     * Blends a type-1 overlay animation onto the orientations for a frame: rotations are multiplied on, translations added
     * and scales multiplied. Untouched nodes and components are left unchanged; other animation types do nothing.
     *
     * @address 0x4d4f90
     */
    void overlay_frame_orientations(int16_t frame, real_orientation *out_orientations);

    /**
     * Same blend as the unweighted overlay, scaled by the weight: rotation is lerped toward identity, translation scaled
     * and scale lerped toward 1.0 before being applied.
     *
     * @address 0x4d51a0
     */
    void overlay_frame_orientations_weighted(int16_t frame, float weight, real_orientation *out_orientations);

    /**
     * Same overlay blend for a fractional frame: interpolates between the two integer frames on either side of it, the
     * second wrapping to 0 past the last frame. Does nothing unless the animation is type 1.
     *
     * @address 0x4d53f0
     */
    void overlay_interpolated_frame_orientations(float frame, real_orientation *out_orientations);

    /**
     * Fractional-frame overlay blend additionally scaled by the weight, with the same weighting rules as the integer-frame
     * weighted overlay.
     *
     * @address 0x4d57d0
     */
    void overlay_interpolated_frame_orientations_weighted(float frame, float weight, real_orientation *out_orientations);

    /**
     * Overwrites only the rotation, translation and scale fields that a type-2 replacement animation animates for the
     * frame. Does nothing for other types or an out-of-range frame.
     *
     * @address 0x4d4dd0
     */
    void replace_frame_orientations(int16_t frame, real_orientation *out_orientations);

    /**
     * Bilinearly blends the four corner keyframes of a 2D (yaw by pitch) aiming overlay grid into the output orientations.
     * Rotation is composed onto the existing per-node rotation and translation is added. Out-of-range input or a
     * non-overlay animation does nothing.
     *
     * @address 0x4d5c00
     */
    void aiming_screen_blend(animation_aiming_screen *screen, real yaw, real pitch, real_orientation *orientation_out);

private:
    ModelAnimationsAnimation *self;
};

}  // namespace halo::models
