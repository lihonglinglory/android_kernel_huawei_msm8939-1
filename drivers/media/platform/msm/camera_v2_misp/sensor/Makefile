#/* < DTS2014122203451 yangyang 20141223 begin */
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/msm_vb2
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/camera
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor/io
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor/cci
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor/mini_isp
obj-$(CONFIG_MSMB_CAMERA) += cci/ io/ csiphy/ csid/ actuator/ flash/ eeprom/ ois/ mini_isp/
#/* DTS2014122203451 yangyang 20141223 end > */
#/* <DTS2014051302540 zhuchengming 20140513 begin */
#/*<DTS2014061904065 Zhangbo 20140619 begin*/
#DTS2015020904827 yuguangcai 20150209 begin
obj-$(CONFIG_MSM_CAMERA_SENSOR) += msm_sensor_init.o msm_sensor_driver.o msm_sensor.o sensor_otp_common_if.o sensor_otp_common.o sensor_otp_ov5648_sunny_p5v18g.o sensor_otp_s5k4e1_liteon_13p1.o sensor_otp_ov5648_foxconn_sc0602.o sensor_otp_s5k4e1_sunny_p5s07a.o
#DTS2015020904827 yuguangcai 20150209 end
#/* DTS2014061904065 Zhangbo 20140619 end >*/
#/* DTS2014051302540 zhuchengming 20140513 end> */
#/* <DTS2014061204421 yangzhenxi/WX221546 20140612 begin */
obj-$(CONFIG_MSM_CAMERA_SENSOR) += sensor_otp_ov8858_foxconn.o
#/* <DTS2014061204421 yangzhenxi/WX221546 20140612 end */

#/* <DTS2014070903091 jiweifeng/jwx206032 20140709 begin */
obj-$(CONFIG_MSM_CAMERA_SENSOR) += sensor_otp_ov13850_sunny_p13v01h.o
#/* DTS2014070903091 jiweifeng/jwx206032 20140709 end> */
#/* <DTS2014081404502 wangguoying 20140814 begin */
obj-$(CONFIG_MSM_CAMERA_SENSOR) += sensor_otp_imx328_sunny_p13n10a.o
#/* DTS2014081404502 wangguoying 20140814 end >*/
obj-$(CONFIG_MT9M114) += mt9m114.o
obj-$(CONFIG_OV5645) += ov5645.o
obj-$(CONFIG_OV5645) += ov7695.o
