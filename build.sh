TARGET_DIR="build_sh"

rm -rf $TARGET_DIR
mkdir $TARGET_DIR && cd $TARGET_DIR
cmake .. \
      -DPAHO_WITH_MQTT_C=TRUE \
      -DPAHO_BUILD_STATIC=TRUE \
      -DPAHO_ENABLE_TESTING=FALSE

make -j8
