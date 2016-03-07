/*
 * Copyright (C) 2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "gazebo/transport/transport.hh"

#include "gazebo/test/ServerFixture.hh"
#include "gazebo/test/helper_physics_generator.hh"

using namespace gazebo;

class PhysicsIfaceTest : public ServerFixture,
    public testing::WithParamInterface<const char*>
{
  /// \brief
  public: void RemoveWorldTest(const std::string &_physicsEngine);
};

/////////////////////////////////////////////////
unsigned int WorldTopicCount(std::map<std::string, std::list<std::string>>
    &_msgTypes)
{
  unsigned int count = 0;
  for (auto msgType : _msgTypes)
  {
    for (auto topic : msgType.second)
    {
      if (topic.find("/gazebo/default") != std::string::npos)
      {
        count++;
      }
    }
  }
  return count;
}

/////////////////////////////////////////////////
void PhysicsIfaceTest::RemoveWorldTest(const std::string &_physicsEngine)
{
  if (_physicsEngine != "ode")
    return;

  // Load a world with some models
  // TODO: world with sensors?
  this->Load("test/worlds/gps_test.world", false, _physicsEngine);

  // Give time for everything to be created
  int sleep = 0;
  int maxSleep = 10;
  while (sleep < maxSleep)
  {
    gazebo::common::Time::MSleep(300);
    sleep++;
  }

  // Get world pointer
  auto world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  auto worldPtrCount = world.use_count();
  EXPECT_GT(worldPtrCount, 1);

  // Get physics engine pointer
  auto physicsEngine = world->GetPhysicsEngine();
  ASSERT_TRUE(physicsEngine != NULL);

  auto physicsEnginePtrCount = physicsEngine.use_count();
  EXPECT_GT(physicsEnginePtrCount, 1);

  // Check advertised topics
  auto msgTypes = gazebo::transport::getAdvertisedTopics();
  EXPECT_FALSE(msgTypes.empty());

  auto worldTopicCount = WorldTopicCount(msgTypes);
  EXPECT_GT(worldTopicCount, 0);

  // Stats before removing world
  gzdbg << std::endl
        << "- WorldPtr use count: [" << world.use_count() << "]" << std::endl
        << "- PhysicsEnginePtr use count: [" << physicsEngine.use_count() << "]"
        << std::endl << "- Topics in this world: [" << worldTopicCount << "]"
        << std::endl;

  // Remove world
  physics::remove_worlds();

  // Give time for everything to be removed
  sleep = 0;
  while (sleep < maxSleep)
  {
    gazebo::common::Time::MSleep(300);
    sleep++;
  }

  // Check the only shared pointer left to the physics engine is this one
  EXPECT_LT(physicsEngine.use_count(), physicsEnginePtrCount);
  EXPECT_EQ(physicsEngine.use_count(), 1);

  // Release the last physics engine pointer
  physicsEngine.reset();

  // Check the only pointer left to the world is this one
  EXPECT_LT(world.use_count(), worldPtrCount);
  EXPECT_EQ(world.use_count(), 1);

  // Release the last world pointer
  world.reset();

  // Check we can't get the world pointer
  try
  {
    world = physics::get_world("default");
  }
  catch(...)
  {
  }
  ASSERT_TRUE(world == NULL);

  // Check all topics related to that world are gone
  msgTypes = gazebo::transport::getAdvertisedTopics();
  EXPECT_LT(WorldTopicCount(msgTypes), worldTopicCount);
  EXPECT_EQ(WorldTopicCount(msgTypes), 0);
}

/////////////////////////////////////////////////
TEST_P(PhysicsIfaceTest, RemoveWorldTest)
{
  RemoveWorldTest(GetParam());
}

INSTANTIATE_TEST_CASE_P(PhysicsIfaces, PhysicsIfaceTest,
                        PHYSICS_ENGINE_VALUES);

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
