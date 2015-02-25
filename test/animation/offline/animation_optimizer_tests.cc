//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//                                                                            //
//============================================================================//

#include "ozz/animation/offline/animation_optimizer.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/math_constant.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/animation_builder.h"

#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/runtime/skeleton.h"

using ozz::animation::Skeleton;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;
using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::AnimationOptimizer;

TEST(Error, AnimationOptimizer) {
  AnimationOptimizer optimizer;

  { // NULL output.
    RawAnimation input;
    Skeleton skeleton;
    EXPECT_TRUE(input.Validate());

    // Builds animation
    EXPECT_FALSE(optimizer(input, skeleton, NULL));
  }

  { // Invalid input animation.
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    SkeletonBuilder skeleton_builder;
    Skeleton* skeleton = skeleton_builder(raw_skeleton);
    ASSERT_TRUE(skeleton != NULL);

    RawAnimation input;
    input.duration = -1.f;
    EXPECT_FALSE(input.Validate());

    // Builds animation
    RawAnimation output;
    output.duration = -1.f;
    output.tracks.resize(1);
    EXPECT_FALSE(optimizer(input, *skeleton, &output));
    EXPECT_FLOAT_EQ(output.duration, RawAnimation().duration);
    EXPECT_EQ(output.num_tracks(), 0);

    ozz::memory::default_allocator()->Delete(skeleton);
  }

  { // Invalid skeleton.
    Skeleton skeleton;

    RawAnimation input;
    input.tracks.resize(1);
    EXPECT_TRUE(input.Validate());

    // Builds animation
    RawAnimation output;
    EXPECT_FALSE(optimizer(input, skeleton, &output));
    EXPECT_FLOAT_EQ(output.duration, RawAnimation().duration);
    EXPECT_EQ(output.num_tracks(), 0);
  }
}

TEST(Optimize, AnimationOptimizer) {
  // Prepares a skeleton.
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  SkeletonBuilder skeleton_builder;
  Skeleton* skeleton = skeleton_builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);

  AnimationOptimizer optimizer;

  RawAnimation input;
  input.duration = 1.f;
  input.tracks.resize(1);
  {
    RawAnimation::TranslationKey key = {
      0.f, ozz::math::Float3(0.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .25f, ozz::math::Float3(.1f, 0.f, 0.f)};  // Not interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .5f, ozz::math::Float3(0.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .625f, ozz::math::Float3(.1f, 0.f, 0.f)};  // Interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .75f, ozz::math::Float3(.21f, 0.f, 0.f)};  // Interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .875f, ozz::math::Float3(.29f, 0.f, 0.f)};  // Interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .9999f, ozz::math::Float3(.4f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      1.f, ozz::math::Float3(0.f, 0.f, 0.f)};  // Last key.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      0.f, ozz::math::Quaternion::identity()};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      .5f, ozz::math::Quaternion::FromEuler(
        ozz::math::Float3(1.1f * ozz::math::kPi / 180.f, 0, 0))};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      1.f, ozz::math::Quaternion::FromEuler(
        ozz::math::Float3(2.f * ozz::math::kPi / 180.f, 0, 0))};
    input.tracks[0].rotations.push_back(key);
  }

  ASSERT_TRUE(input.Validate());

  // Builds animation with very little tolerance.
  {
    optimizer.translation_tolerance = 0.f;
    optimizer.rotation_tolerance = 0.f;
    RawAnimation output;
    ASSERT_TRUE(optimizer(input, *skeleton, &output));
    EXPECT_EQ(output.num_tracks(), 1);

    const RawAnimation::JointTrack::Translations& translations =
      output.tracks[0].translations;
    ASSERT_EQ(translations.size(), 8u);
    EXPECT_FLOAT_EQ(translations[0].value.x, 0.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(translations[1].value.x, .1f);  // Track 0 at .25f.
    EXPECT_FLOAT_EQ(translations[2].value.x, 0.f);  // Track 0 at .5f.
    EXPECT_FLOAT_EQ(translations[3].value.x, .1f);  // Track 0 at .625f.
    EXPECT_FLOAT_EQ(translations[4].value.x, .21f);  // Track 0 at .75f.
    EXPECT_FLOAT_EQ(translations[5].value.x, .29f);  // Track 0 at .875f.
    EXPECT_FLOAT_EQ(translations[6].value.x, .4f);  // Track 0 ~end.
    EXPECT_FLOAT_EQ(translations[7].value.x, 0.f);  // Track 0 end.*/

    const RawAnimation::JointTrack::Rotations& rotations =
      output.tracks[0].rotations;
    ASSERT_EQ(rotations.size(), 3u);
    EXPECT_FLOAT_EQ(rotations[0].value.w, 1.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(rotations[1].value.w, .9999539f);  // Track 0 at .5f.
    EXPECT_FLOAT_EQ(rotations[2].value.w, .9998477f);  // Track 0 end.
  }
  {
    // Rebuilds with tolerance.
    optimizer.translation_tolerance = .02f;
    optimizer.rotation_tolerance = .2f * 3.14159f / 180.f;  // .2 degree.
    RawAnimation output;
    ASSERT_TRUE(optimizer(input, *skeleton, &output));
    EXPECT_EQ(output.num_tracks(), 1);

    const RawAnimation::JointTrack::Translations& translations =
      output.tracks[0].translations;
    ASSERT_EQ(translations.size(), 5u);
    EXPECT_FLOAT_EQ(translations[0].value.x, 0.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(translations[1].value.x, .1f);  // Track 0 at .25f.
    EXPECT_FLOAT_EQ(translations[2].value.x, 0.f);  // Track 0 at .5f.
    EXPECT_FLOAT_EQ(translations[3].value.x, 0.4f);  // Track 0 at ~1.f.
    EXPECT_FLOAT_EQ(translations[4].value.x, 0.f);  // Track 0 end.

    const RawAnimation::JointTrack::Rotations& rotations =
      output.tracks[0].rotations;
    ASSERT_EQ(rotations.size(), 2u);
    EXPECT_FLOAT_EQ(rotations[0].value.w, 1.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(rotations[1].value.w, .9998477f);  // Track 0 end.
  }

  ozz::memory::default_allocator()->Delete(skeleton);
}

TEST(OptimizeHierarchical, AnimationOptimizer) {
  // Prepares a skeleton.
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  raw_skeleton.roots[0].children.resize(1);
  raw_skeleton.roots[0].children[0].children.resize(1);
  SkeletonBuilder skeleton_builder;
  Skeleton* skeleton = skeleton_builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);

  AnimationOptimizer optimizer;

  RawAnimation input;
  input.duration = 1.f;
  input.tracks.resize(3);

  // Translations on track 0.
  {
    RawAnimation::TranslationKey key = {
      0.f, ozz::math::Float3(0.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .1f, ozz::math::Float3(1.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .2f, ozz::math::Float3(2.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .3f, ozz::math::Float3(3.001f, 0.f, 0.f)};  // Creates an error.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .4f, ozz::math::Float3(4.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }

  // Rotations on track 0.
  {
    RawAnimation::RotationKey key = {
      0.f, ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f)};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      .1f, ozz::math::Quaternion(0.f, 0.f, .70710678118654f, .70710678118654f)};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      .2f, ozz::math::Quaternion(0.f, 0.f, 1.f, 0.f)};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      .3f, ozz::math::Quaternion(0.f, 0.f, .70710670f, .70710686237308f)};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      .4f, ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f)};
    input.tracks[0].rotations.push_back(key);
  }

  // Scales on track 0.
  {
    RawAnimation::ScaleKey key = {
      0.f, ozz::math::Float3(0.f, 1.f, 1.f)};
    input.tracks[0].scales.push_back(key);
  }
  {
    RawAnimation::ScaleKey key = {
      .1f, ozz::math::Float3(1.f, 1.f, 1.f)};
    input.tracks[0].scales.push_back(key);
  }
  {
    RawAnimation::ScaleKey key = {
      .2f, ozz::math::Float3(2.f, 1.f, 1.f)};
    input.tracks[0].scales.push_back(key);
  }
  {
    RawAnimation::ScaleKey key = {
      .3f, ozz::math::Float3(3.001f, 1.f, 1.f)};
    input.tracks[0].scales.push_back(key);
  }
  {
    RawAnimation::ScaleKey key = {
      .4f, ozz::math::Float3(4.f, 1.f, 1.f)};
    input.tracks[0].scales.push_back(key);
  }

  // Translations on track 1 have a big length which impact rotation
  // optimizations.
  {
    RawAnimation::TranslationKey key = {
      0.f, ozz::math::Float3(0.f, 0.f, 1000.f)};
    input.tracks[1].translations.push_back(key);
  }

  // Scales on track 2 have a big scale which impact translation
  // optimizations.
  {
    RawAnimation::ScaleKey key = {
      0.f, ozz::math::Float3(10.f, 100.f, 1000.f)};
    input.tracks[2].scales.push_back(key);
  }

  ASSERT_TRUE(input.Validate());

  // Builds animation with very default tolerance.
  {
    RawAnimation output;
    ASSERT_TRUE(optimizer(input, *skeleton, &output));
    EXPECT_EQ(output.num_tracks(), 3);

    const RawAnimation::JointTrack::Translations& translations =
      output.tracks[0].translations;
    ASSERT_EQ(translations.size(), 4u);
    EXPECT_FLOAT_EQ(translations[0].value.x, 0.f);
    EXPECT_FLOAT_EQ(translations[1].value.x, 2.f);
    EXPECT_FLOAT_EQ(translations[2].value.x, 3.001f);
    EXPECT_FLOAT_EQ(translations[3].value.x, 4.f);

    const RawAnimation::JointTrack::Rotations& rotations =
      output.tracks[0].rotations;
    ASSERT_EQ(rotations.size(), 4u);
    EXPECT_FLOAT_EQ(rotations[0].value.w, 1.f);
    EXPECT_FLOAT_EQ(rotations[1].value.w, 0.f);
    EXPECT_FLOAT_EQ(rotations[2].value.w, .70710686237308f);
    EXPECT_FLOAT_EQ(rotations[3].value.w, 1.f);
    
    const RawAnimation::JointTrack::Scales& scales =
      output.tracks[0].scales;
    ASSERT_EQ(rotations.size(), 4u);
    EXPECT_FLOAT_EQ(scales[0].value.x, 0.f);
    EXPECT_FLOAT_EQ(scales[1].value.x, 2.f);
    EXPECT_FLOAT_EQ(scales[2].value.x, 3.001f);
    EXPECT_FLOAT_EQ(scales[3].value.x, 4.f);
  }

  ozz::memory::default_allocator()->Delete(skeleton);
}
