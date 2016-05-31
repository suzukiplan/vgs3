# SUZUKI PLAN - Video Game System Mark-III
[WIP] SUZUKI PLAN - Video Game System Mark-III

## description
### architecture
![architecture](https://raw.githubusercontent.com/suzukiplan/vgs3/master/image/architecture.png)

### difference between VGS mk-II SR
#### separation of the game and app
- 従来の VGS は ROMDATA.BIN にアセットしか含まなかったが, mk-III では プログラム が含まれます
- mk-III のプログラムは, 既存のCPU向けのマシン語ではなく独自の 32bit CPU（VGS-CPU）が解釈実行できるマシン語で記述する
- VGS-CPU向けのマシン語は独自アセンブリ言語（VGS-ASM）でアセンブルできる

#### VGS-CPU
- データモデル: 32bit little endian
- クロック周波数: 制限しない（実行環境依存）
- 特徴
  - stack / main-memory / program のメモリ領域が分離している
  - VGS-API を実行する専用の命令（ニモニックVGS）がある
  - 一般的なCPUと比べて、アセンブリ言語のみでのゲームプログラムの記述が容易 _(ファミコンのRP2A03よりも容易)_

#### structure of ROMDATA.BIN
- BSLOT: BGMデータ（音楽）が格納されるスロット
- ESLOT: EFFデータ（効果音）が格納されるスロット
- GSLOT: CHRデータ（画像）が格納されるスロット
- PSLOT: BINデータ（マシン語）が格納されるスロット
- DSLOT: 任意データが格納されるスロット
