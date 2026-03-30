- Ballance mod, based on [Ballance Mod Loader Plus](https://github.com/doyaGu/BallanceModLoaderPlus)
## Logs
- 20260329: finished ControlDirCamera
- 20260324: fixed issue: [RainMode] If a section stays for more than 4 minutes, the frame rate will drop significantly.
- 20260324: Recode and optimized the RainMode
- 20260322: upload NewSpawn again and implemented a new feature that randomizes the ball's orientation.



## NewSpawn
- A new custom spawn point mod (similar to the built-in `/spawn` command in BML but more powerful). Supports resetting to different segments, the same segment without resetting mechanisms or shadow orb recordings, making speedrun practice more convenient.
- Usage: [NewSpawn](https://github.com/Xenapte/ballance-mod-docs-zh/wiki/%E6%96%87%E6%A1%A3#newspawn)
## RainMode
- Fun mod, Items can be generated to fall from the sky, with multiple adjustable parameters.
- <details>
    <summary>Usage in Chinese</summary>

    -  Time_Interval: 每波物件的生成时间间隔
    -  Entities_Interval: 每波生成多少个物件
    -  Rain_Region: 生成区域的边长
    -  Rain_Height: 雨生成的高度
    -  Ball_Speed_Correlation: 生成区域会多大程度受到玩家球速的影响
    -  Intensity_State: 对道具施加初始冲量的强度
    -  Entities_Capacity: 道具容量，超出容量后，最先生成的道具会被清除
    -  Entities_Proportion: 不同物件的生成比例
    -  默认值相对比较激烈，可以通过调以上数据来降低难度，建议调整本说明中的前4个数据
</details>

## ControlDirCamera
- 这是个用于TAS观察球位的mod，视角随实际操作方向变化，可自定义视角变化，变化的视角不会影响操作施力方向。可以用于解决转视角时计算球实际操作方向的烦琐。可能类似[StaticCamera](https://github.com/Xenapte/MyBMLMods/tree/main/StaticCamera)但实际效果和可自定义程度应该会好不少（应该）
- <details>
    <summary>Usage in Chinese</summary>

    -  Show_Data: 开启后在游戏界面右上角查看相机详细数据
    -  Camera_Fov: 更改摄像机的视野范围
    -  Camera_Position_Right: 以右为正方向，相机绕球水平旋转多少角度（可填0-360）
    -  Camera_Orient_UP: 相机视线的竖直方向参数（可填0-90），填得越高视线越平，看得越远。
    -  Camera_Height: 相机位置高度
    -  Camera_Distance: 相机在水平方向上距离球的距离
    -  通过调整 Camera_Orient_UP(90)、Camera_Height(5) 以及 Camera_Position_Right(150) 可以让相机看到狭小范围内的东西（比如6-2遮挡）
</details>

## DrunkMode
- 一个模拟喝醉了的感觉的mod，逻辑比较简陋，只有摄像机随机摇晃和随机力作用到玩家球上。参数改大可能会有一些有趣的画面
- <details>
    <summary>Usage in Chinese</summary>

    -  Camera_Enabled: 启用摄像机摇晃效果
    -  Camera_Intensity: 相机整体效果强度
    -  Camera_Frequency: 相机晃动的速度大小
    -  Camera_Displacement: 相机位移幅度
    -  Force_Enabled: 启用球失控效果（随机作用力）
    -  Force_Intensity: 作用力强度
    -  Force_Frequency: 作用力变化快慢
    -  所有值均默认1，可设置大于0的小数值。
</details>

## AdditionalSkills
- just coding