#include "main.h"

#include "mcap/writer.hpp"
#include "foxglove/schema.h"

#include "foxglove/PackedElementField_generated.h"
#include "foxglove/Time_generated.h"
#include "foxglove/PointCloud_generated.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <random>

// Example from
// https://github.com/foxglove/mcap/blob/8562339b0c37b6cc3145991d0b3b7c02a6e97dce/cpp/examples/protobuf/writer.cpp
// Ported to flatbuffers

#define NS_PER_MS 1000000
#define NS_PER_S 1000000000
#define POINTS_PER_CLOUD 1000
#define FIELDS_PER_POINT 3

FOXGLOVE_SCHEMA(PointCloud)

// PointGenerator generates random points on a sphere.
class PointGenerator {
  private:
    std::mt19937 _generator;
    std::uniform_real_distribution<float> _distribution;
  public:
    PointGenerator(uint32_t seed = 0)
      : _generator(seed), _distribution(0.0, 1.0) {}

    // next produces a random point on the unit sphere, scaled by `scale`.
    std::tuple<float, float, float> next(float scale) {
      float theta = 2 * static_cast<float>(M_PI) * _distribution(_generator);
      float phi = acos(1.f - (2.f * _distribution(_generator)));
      float x = float((sin(phi) * cos(theta)) * scale);
      float y = float((sin(phi) * sin(theta)) * scale);
      float z = float(cos(phi) * scale);
      return {x, y, z};
    }
};

void initialize() {
  const char* outputFilename = "/usd/test.mcap";
  printf("Hello, PROS!\n");
  mcap::McapWriter writer;
  {
    auto options = mcap::McapWriterOptions("");
    const auto res = writer.open(outputFilename, options);
    if (!res.ok()) {
      std::cerr << "Failed to open " << outputFilename
                << " for writing: " << res.message << std::endl;
      return;
    }
  }
  printf("Opened file\n");

  // Create a channel and schema for our messages.
  // A message's channel informs the reader on the topic those messages were
  // published on. A channel's schema informs the reader on how to interpret the
  // messages' content. MCAP follows a relational model, where:
  // * messages have a many-to-one relationship with channels (indicated by
  // their channel_id)
  // * channels have a many-to-one relationship with schemas (indicated by their
  // schema_id)
  mcap::ChannelId channelId;
  {
    // std::vector<std::byte> schemaBytes(
    //     (std::byte*)foxglove_PointCloud_bfbs.buf,
    //     (std::byte*)foxglove_PointCloud_bfbs.buf +
    //         foxglove_PointCloud_bfbs.size);

    // flatbuffer schemas in MCAP are represented as a serialized flat buffer
    // schema These are auto generated with the flatc compiler from a .fbs file.
    // mcap::Schema schema("foxglove.PointCloud", "flatbuffer",
    //                     foxglove::schemas::PointCloud);
    writer.addSchema(foxglove::schemas::PointCloud);

    // choose an arbitrary topic name.
    mcap::Channel channel("pointcloud", "flatbuffer",
                          foxglove::schemas::PointCloud.id);
    writer.addChannel(channel);
    channelId = channel.id;
  }
  printf("Created channel\n");

  flatbuffers::FlatBufferBuilder builder(
      POINTS_PER_CLOUD * FIELDS_PER_POINT * sizeof(float) + 64);

  printf("Created builder\n");

  mcap::Timestamp startTime =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  PointGenerator pointGenerator;
  // write 100 pointcloud messages into the output MCAP file.
  for (uint32_t frameIndex = 0; frameIndex < 100; ++frameIndex) {
    // Space these frames 100ms apart in time.
    mcap::Timestamp cloudTime =
        startTime + (static_cast<uint64_t>(frameIndex) * 100 * NS_PER_MS);
    // Slightly increase the size of the cloud on every frame.
    float cloudScale = 1.f + (static_cast<float>(frameIndex) / 50.f);

    std::array<flatbuffers::Offset<foxglove::PackedElementField>, 3> fields;
    {
      // Describe the data layout to the consumer of the pointcloud. There are 3
      // single-precision float fields per point.
      const char* const fieldNames[] = {"x", "y", "z"};
      static_assert(sizeof(fieldNames) / sizeof(fieldNames[0]) ==
                        sizeof(fields) / sizeof(fields[0]),
                    "field arrays do not have equal length");

      int fieldOffset = 0;
      for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        auto fieldName = builder.CreateString(fieldNames[i]);
        auto field = foxglove::CreatePackedElementField(
            builder, fieldName, fieldOffset, foxglove::NumericType_FLOAT32);
        fields[i] = field;
        fieldOffset += sizeof(float);
      }
    }

    std::vector<float> pointData;
    // write 1000 points into each pointcloud message.
    for (int pointIndex = 0; pointIndex < POINTS_PER_CLOUD; ++pointIndex) {
      auto [x, y, z] = pointGenerator.next(cloudScale);
      pointData.push_back(x);
      pointData.push_back(y);
      pointData.push_back(z);
    }

    auto data =
        builder.CreateVector(reinterpret_cast<const uint8_t*>(pointData.data()),
                             pointData.size() * sizeof(float));
    auto fieldVector = builder.CreateVector(fields.begin(), fields.size());
    auto frameId = builder.CreateString("pointcloud");

    auto pose =
        foxglove::CreatePose(builder, foxglove::CreateVector3(builder, 0, 0, 0),
                             foxglove::CreateQuaternion(builder, 0, 0, 0, 1));
    const auto timestamp =
        foxglove::Time(static_cast<int64_t>(cloudTime) / NS_PER_S,
                       static_cast<int32_t>(cloudTime % NS_PER_S));

    auto pointCloud = foxglove::CreatePointCloud(
        builder, &timestamp, frameId, pose, sizeof(float) * FIELDS_PER_POINT,
        fieldVector, data);
    builder.Finish(pointCloud);

    // Include the pointcloud data in a message, then write it into the MCAP
    // file.
    mcap::Message msg;
    msg.channelId = channelId;
    msg.sequence = frameIndex;
    msg.publishTime = cloudTime;
    msg.logTime = cloudTime;
    msg.data = (const std::byte*)(builder.GetBufferPointer());
    msg.dataSize = builder.GetSize();
    const auto res = writer.write(msg);
    if (!res.ok()) {
      std::cerr << "Failed to write message: " << res.message << "\n";
      writer.terminate();
      std::ignore = std::remove(outputFilename);
      return;
    }

    builder.Clear();

    printf("Wrote frame: %i\n", frameIndex);
  }
  // Write the index and footer to the file, then close it.
  writer.close();
  std::cerr << "wrote 100 pointcloud messages to " << outputFilename
            << std::endl;
  printf("Closed file\n");
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol() {
  pros::Controller master(pros::E_CONTROLLER_MASTER);
  pros::MotorGroup left_mg({1, -2, 3}); // Creates a motor group with forwards
                                        // ports 1 & 3 and reversed port 2
  pros::MotorGroup right_mg({-4, 5, -6}); // Creates a motor group with forwards
                                          // port 5 and reversed ports 4 & 6

  while (true) {
    pros::lcd::print(0, "%d %d %d",
                     (pros::lcd::read_buttons() & LCD_BTN_LEFT) >> 2,
                     (pros::lcd::read_buttons() & LCD_BTN_CENTER) >> 1,
                     (pros::lcd::read_buttons() & LCD_BTN_RIGHT) >>
                         0); // Prints status of the emulated screen LCDs

    // Arcade control scheme
    int dir = master.get_analog(
        ANALOG_LEFT_Y); // Gets amount forward/backward from left joystick
    int turn = master.get_analog(
        ANALOG_RIGHT_X); // Gets the turn left/right from right joystick
    left_mg.move(dir - turn); // Sets left motor voltage
    right_mg.move(dir + turn); // Sets right motor voltage
    pros::delay(20); // Run for 20 ms then update
  }
}