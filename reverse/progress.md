# 文件
所有IMG后缀文件格式统一，所以他们处理完一个，就全部分析完成。
Dat后缀的文件一共四种情况：
1. 自带图形数据及调色板
2. 自带图形数据，不带有调色板。需要逆向分析其调色板。
3. 文本文件（txt.dat)，加了密
4. 音频文件（wave.dat)

FLC后缀文件好像也是加密的，解读中....

# 文件进度

*宽度、高度在img里面的值都比实际值小1*

| 名称 | 进度 | 宽度 | 高度 | 数量 | 模板 | 说明 |
| ---  | --- | --- | :-- | :-- | :-- | :-- |
|77-1.IMG|![%100](https://progress-bar.dev/100/)| 640 | 428 | 1 | dlz_img.bt | |
|77-2.IMG|![%100](https://progress-bar.dev/100/)| 580 | 480 | 1 | dlz_img.bt | |
|918-2.IMG|![%100](https://progress-bar.dev/100/)| 640 | 276 | 1 | dlz_img.bt | |
|918-3.IMG|![%100](https://progress-bar.dev/100/)| 640 | 367 | 1 | dlz_img.bt | |
|ARMY.DAT|![%100](https://progress-bar.dev/100/)| 48 | 48 | 19*91 | dlz_army.bt | 每个兵种19帧，共91个兵种 |
|ASC16|![%0](https://progress-bar.dev/0/)| | | |  | 不予处理 |
|BACK19.IMG|![%100](https://progress-bar.dev/100/)| 640 | 640 | 1 | dlz_img.bt | |
|BG01.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | |
|BG02.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | |
|BG03.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | |
|BIGMAP.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 平原县的总地图 |
|BIGMINE0.IMG|![%100](https://progress-bar.dev/100/)| 70 | 81 | 1 | dlz_img.bt | |
|BIGMINE1.IMG|![%100](https://progress-bar.dev/100/)| 70 | 81 | 1 | dlz_img.bt | |
|BLK.DAT|![%100](https://progress-bar.dev/100/)| 48 | 48 | | dlz_blk.bt | 战斗场景的塔图 |
|BOX.IMG|![%100](https://progress-bar.dev/100/)| 48 | 288 | 1 | dlz_img.bt | |
|BUTTON0.IMG|![%100](https://progress-bar.dev/100/)| 50 | 25 | 1 | dlz_img.bt | |
|BUTTON00.IMG|![%100](https://progress-bar.dev/100/)| 28 | 312 | 1 | dlz_img.bt | |
|BUTTON00.TGA|![%100](https://progress-bar.dev/100/)| 28 | 312 | 1 | tga.bt | |
|BUTTON01.IMG|![%100](https://progress-bar.dev/100/)| 28 | 312 | 1 | dlz_img.bt | |
|BUTTON01.TGA|![%100](https://progress-bar.dev/100/)| 28 | 312 | 1 | tga.bt | |
|BUTTON1.IMG|![%100](https://progress-bar.dev/100/)| 50 | 25 | 1 | dlz_img.bt | |
|BUTTON10.IMG|![%100](https://progress-bar.dev/100/)| 35 | 19 | 1 | dlz_img.bt | |
|BUTTON11.IMG|![%100](https://progress-bar.dev/100/)| 35 | 19 | 1 | dlz_img.bt | |
|BUTTON12.IMG|![%100](https://progress-bar.dev/100/)| 35 | 19 | 1 | dlz_img.bt | |
|BUTTON13.IMG|![%100](https://progress-bar.dev/100/)| 35 | 19 | 1 | dlz_img.bt | |
|BUTTON2.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|BUTTON22.IMG|![%100](https://progress-bar.dev/100/)| 88 | 64 | 1 | dlz_img.bt | |
|BUTTON3.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|BUTTON4.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|BUTTON5.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|BUTTON6.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|BUTTON7.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|BUTTON8.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|BUTTON9.IMG|![%100](https://progress-bar.dev/100/)| 48 | 44 | 1 | dlz_img.bt | |
|CIRCLE.TGA|![%100](https://progress-bar.dev/100/)| 48 | 48 | 1 | tga.bt | |
|DLZ.exe|![%0](https://progress-bar.dev/0/)| | | | ida.exe | 这是一个漫长的过程 |
|DLZ00.IMG|![%100](https://progress-bar.dev/100/)| 40 | 360 | 1 | dlz_img.bt | |
|DLZ00.TGA|![%100](https://progress-bar.dev/100/)| 40 | 360 | 1 | tga.bt | |
|DLZ01.TGA|![%100](https://progress-bar.dev/100/)| 40 | 360 | 1 | tga.bt | |
|END00.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | |
|END01.IMG|![%100](https://progress-bar.dev/100/)| 320 | 200 | 1 | dlz_img.bt | |
|END02.IMG|![%100](https://progress-bar.dev/100/)| 320 | 200 | 1 | dlz_img.bt | |
|END03.IMG|![%100](https://progress-bar.dev/100/)| 320 | 200 | 1 | dlz_img.bt | |
|END04.IMG|![%0](https://progress-bar.dev/0/)|  |  |  | dlz_img.bt | 数据异常？ |
|END05.IMG|![%100](https://progress-bar.dev/100/)| 320 | 200 | 1 | dlz_img.bt | |
|FACE.DAT|![%100](https://progress-bar.dev/100/)| 80 | 100 | 170 | dlz_face.bt | |
|FFPACT00.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT01.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT02.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT03.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT04.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT05.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT06.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT07.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT08.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT09.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT10.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT11.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT12.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT13.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT41.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT42.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT43.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT44.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT45.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT46.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT47.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT48.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT49.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT50.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT51.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT52.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT53.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FFPACT54.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|FIGHT.DAT|![%50](https://progress-bar.dev/50/)| | | | dlz_fight.bt | 这个比较复杂 |
|FLM.DAT|![%100](https://progress-bar.dev/100/)| 48 | 48 | 16*32 | dlz_flm.bat | 共32个动画，每个动画16帧 |
|FRAME00.IMG|![%100](https://progress-bar.dev/100/)| 404 | 127 | 1 | dlz_img.bt | |
|FRAME00.TGA|![%100](https://progress-bar.dev/100/)| 404 | 127 | 1 | tga.bt | |
|GAME.DAT|![%70](https://progress-bar.dev/70/)| | | | dlz_game.bt | 游戏基础数据 |
|GOODS.DAT|![%100](https://progress-bar.dev/100/)| | | | dlz_goods.bt | 商品数据 |
|HZK16|![%0](https://progress-bar.dev/0/)| | | | | 不予处理 |
|JZ-1.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 剧终1 |
|JZ-2.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 剧终2 |
|K00.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框1 |
|K01.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框2 |
|K02.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框3 |
|K03.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框4 |
|K04.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框5 |
|KINGSOFT.FLC|![%0](https://progress-bar.dev/0/)| | | | | |
|KR-1.FLC|![%0](https://progress-bar.dev/0/)| | | | | |
|KUAN04.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框6 |
|KUANG00.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框7 |
|KUANG01.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框8 |
|KUANG04.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 框9 |
|MAP.DAT|![%100](https://progress-bar.dev/100/)| | | | dlz_map.bt | 战斗地图，共48，设计上限64 |
|MATERIAL.DAT|![%100](https://progress-bar.dev/100/)| 48 | 48 | 10 | dlz_material.bt | 地雷材料 |
|MENU.TGA|![%100](https://progress-bar.dev/100/)| 80 | 120 | 1 | tga.bt | 菜单背景 |
|MENU00.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | tga.bt | 战报背景 |
|MENU01.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | tga.bt | 主角生成背景 |
|MENU02.IMG|![%100](https://progress-bar.dev/100/)| 480 | 360 | 1 | tga.bt | 背景2 |
|MENU03.IMG|![%100](https://progress-bar.dev/100/)| 128 | 88 | 1 | tga.bt | 菜单3 |
|MENU08.IMG|![%100](https://progress-bar.dev/100/)| 304 | 227 | 1 | tga.bt | 背景8 |
|MENU09.IMG|![%100](https://progress-bar.dev/100/)| 456 | 227 | 1 | tga.bt | 背景8 |
|MENU10.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | tga.bt | 战报背景 |
|MENU20.IMG|![%100](https://progress-bar.dev/100/)| 168 | 48 | 1 | tga.bt | 初出茅庐 |
|MENU21.IMG|![%100](https://progress-bar.dev/100/)| 168 | 48 | 1 | tga.bt | 继往开来 |
|MENU22.IMG|![%100](https://progress-bar.dev/100/)| 168 | 48 | 1 | tga.bt | 战斗英雄 |
|MENU23.IMG|![%100](https://progress-bar.dev/100/)| 168 | 48 | 1 | tga.bt | 支前模范 |
|MENU24.IMG|![%100](https://progress-bar.dev/100/)| 168 | 48 | 1 | tga.bt | 部队休整 |
|MENU30.IMG|![%100](https://progress-bar.dev/100/)| 17 | 25 | 1 | tga.bt | |
|MENU34.IMG|![%100](https://progress-bar.dev/100/)| 304 | 168 | 1 | tag.bt | |
|MENU50.IMG|![%100](https://progress-bar.dev/100/)| 232 | 56 | 1 | tga.bt | 初出茅庐 |
|MENU51.IMG|![100](https://progress-bar.dev/100/)| 232 | 56 | 1 | tga.bt | 继往开来 |
|MENU52.IMG|![100](https://progress-bar.dev/100/)| 232 | 56 | 1 | tga.bt | 战斗英雄 |
|MENU53.IMG|![100](https://progress-bar.dev/100/)| 232 | 56 | 1 | tga.bt | 支前模范 |
|MENU54.IMG|![100](https://progress-bar.dev/100/)| 232 | 56 | 1 | tga.bt | 部队休整 |
|MINE.DAT|![%100](https://progress-bar.dev/100/)| 48 | 48 | 4*10 | dlz_mine.bt | 由10类地雷和4个引爆方式组成的40个地雷图标|
|MINE.IMG|![%100](https://progress-bar.dev/100/)| 16 | 16 | 1 | dlz_img.bt | 地雷小图标 |
|MINE00.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE01.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE01.IMG|![%100](https://progress-bar.dev/100/)| 640 | 480 | 1 | dlz_img.bt | 游戏启动界面 |
|MINE02.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE02.IMG|![%100](https://progress-bar.dev/100/)| 56 | 1440 | 1 | dlz_img.bt | 旋转的地雷 |
|MINE03.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE04.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE05.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE06.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE07.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE08.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|MINE09.ANM|![%0](https://progress-bar.dev/0/)| | | | | |
|NPIC01.IMG|![%100](https://progress-bar.dev/100/)| 640 | 366 | 1 | dlz_img.bt | |
|NPIC02.IMG|![%100](https://progress-bar.dev/100/)| 489 | 480 | 1 | dlz_img.bt | |
|NPIC03.IMG|![%100](https://progress-bar.dev/100/)| 345 | 480 | 1 | dlz_img.bt | |
|NPIC04.IMG|![%100](https://progress-bar.dev/100/)| 571 | 480 | 1 | dlz_img.bt | |
|NPIC05.IMG|![%100](https://progress-bar.dev/100/)| 640 | 449 | 1 | dlz_img.bt | |
|NPIC06.IMG|![%100](https://progress-bar.dev/100/)| 640 | 449 | 1 | dlz_img.bt | |
|NPIC07.IMG|![%100](https://progress-bar.dev/100/)| 640 | 460 | 1 | dlz_img.bt | |
|NPIC08.IMG|![%100](https://progress-bar.dev/100/)| 640 | 439 | 1 | dlz_img.bt | |
|NPIC18.IMG|![%100](https://progress-bar.dev/100/)| 633 | 480 | 1 | dlz_img.bt | |
|NPIC19.IMG|![%100](https://progress-bar.dev/100/)| 640 | 424 | 1 | dlz_img.bt | |
|NPIC20.IMG|![%100](https://progress-bar.dev/100/)| 373 | 480 | 1 | dlz_img.bt | |
|NPIC21.IMG|![%100](https://progress-bar.dev/100/)| 380 | 480 | 1 | dlz_img.bt | |
|NPIC22.IMG|![%100](https://progress-bar.dev/100/)| 353 | 480 | 1 | dlz_img.bt | |
|NPIC25.IMG|![%100](https://progress-bar.dev/100/)| 609 | 275 | 1 | dlz_img.bt | |
|NPIC29.IMG|![%100](https://progress-bar.dev/100/)| 640 | 445 | 1 | dlz_img.bt | |
|PEOPLE.DAT|![%90](https://progress-bar.dev/90/)| | | | dlz_people.bt | 40个人物，包括孙悟空 |
|PLACE.DAT|![%100](https://progress-bar.dev/100/)| 640 | 480 | 19 | dlz_place.bt | 19个城镇的背景图片 |
|Test01.flc|![%0](https://progress-bar.dev/0/)| | | | | |
|TOWN.DAT|![%20](https://progress-bar.dev/20/)| | | | dlz_town.bt | 19个城镇的数据 |
|TRAIN.FLC|![%0](https://progress-bar.dev/0/)| | | | | |
|TXT.DAT|![%100](https://progress-bar.dev/100/)| | | | show_txt.py | 对话数据，加了密，很简单 |
|WAVE.DAT|![%50](https://progress-bar.dev/50/)| | | | dlz_wave.bt | 音频数据 |
|ZYB.IMG |![%0](https://progress-bar.dev/0/)| | | | dlz_img.bt | 游戏通关图片，比较特殊,采用滚动模式读取？ |