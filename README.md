- Ballance mod, based on [Ballance Mod Loader Plus](https://github.com/doyaGu/BallanceModLoaderPlus)
## Logs
- 20260324: Recode and optimized the RainMode
- 20260322: upload NewSpawn again and implemented a new feature that randomizes the ball's orientation.

## NewSpawn
- A new custom spawn point mod (similar to the built-in `/spawn` command in BML but more powerful). Supports resetting to different segments, the same segment without resetting mechanisms or shadow orb recordings, making speedrun practice more convenient.
- Usage: [NewSpawn](https://github.com/Xenapte/ballance-mod-docs-zh/wiki/%E6%96%87%E6%A1%A3#newspawn)
## RainMode
- Fun mod, Items can be generated to fall from the sky, with multiple adjustable parameters.
- known issues：
  - If a section stays for more than 4 minutes, the frame rate will drop significantly.
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
</details>

## DrunkMode
- just coding

## AdditionalSkills
- just coding