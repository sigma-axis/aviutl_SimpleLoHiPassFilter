# 簡易ローパス・ハイパスフィルタ AviUtl プラグイン

AviUtl の拡張編集に「簡易ローパスハイパス」フィルタ効果とフィルタオブジェクトを追加するプラグインです．音声にローパスフィルタをかけてこもったような音にしたり，ハイパスフィルタをかけて遠くから聞こえているような音にしたりできます．

音声加工の手法自体は簡易的なものなので，凝ったのを誰か知識のある方が作ってくれるのを期待しています...

TODO: video

## 動作要件

- AviUtl 1.10 + 拡張編集 0.92

  http://spring-fragrance.mints.ne.jp/aviutl
  - 拡張編集 0.93rc1 等の他バージョンでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

- patch.aul の `r43 謎さうなフォーク版58` (`r43_ss_58`) 以降

  https://github.com/nazonoSAUNA/patch.aul/releases/latest


## 導入方法

`aviutl.exe` と同階層にある `plugins` フォルダ内に `SimpleLoHiPassFilter.eef` ファイルをコピーしてください．

## 使い方

音声系オブジェクトのフィルタ効果の追加メニューとフィルタオブジェクトの追加メニューに「簡易ローパスハイパス」が追加されています．

TODO: image of UI

- `周波数` でカットオフ周波数を調整します．

  - `±1.00` ごとに 1 半音（1/12 オクターブ）変化します．

  - スライダーの移動範囲は `-24.00` から `+48.00` までですが，直接数値入力（あるいは数値部分を左右にドラッグ移動）で `-48.00` から `+60.00` まで指定できます．初期値は `+12.00`.

  - 内部的には 440 Hz の A (ラ) を起点として計算していますが，あくまでも目安としてとらえてください．

- `強さ` でフィルタの全体的なかかり具合を % 単位で指定します．

  - `0` を指定するとフィルタ効果がかかっていないときと同じ音声になります．`100` でフィルタをかけた音声のみが出力されます．初期値は `100`.

- `補正dB` でフィルタによって小さくなってしまった音を調整できます．

  フィルタをかけるとその計算手法上，音量が全体的に小さく聞こえる傾向が出てきてしまいます．どのくらい小さくなるかは元となる音声の特性やカットオフ `周波数` の設定，`窓関数` の選択や体感などに依存するため，不自然に小さくなってしまった場合は手動で再調整する必要があります．

  - 元音声からの相対音量を dB 単位で指定します．スライダーの移動範囲は `0.00` から `+20.00` までですが，直接数値入力（あるいは数値部分を左右にドラッグ移動）で `-10.00` から `+40.00` まで指定できます．初期値は `0.00`.

  - **調整の際には音が大きくなりすぎないよう注意してください．**
  
    最大値である `+40.00` dB は，元音声の 1 万倍もの大音量になり，音割れも非常に起こりやすくなります．カットオフ周波数などを再調整した場合も，音の大きさが変わっていることがあるため不意に大音量を再生しないように注意してください．

- `最小遅延` で予め設定しておく遅延量をミリ秒単位で指定します．

  過去の音声波形に基づいて計算するという手法の都合上，フィルタをかけていない場合と比べて遅延が発生します（最大想定 40 ミリ秒）．必要最小限の遅延量はカットオフ `周波数` や `窓関数` の選択で変動します．しかしこの変動が原因で常に必要最小限の遅延量にした場合，周波数を時間変化させるとノイズが発生したり体感の再生速度が不均一になったりすることがあります．

  予め遅延量を大きめに設定しておくことで遅延量の変動を防ぎ，ノイズの抑制や体感再生速度を均一化できます．その「大きめに設定」しておく遅延量を指定します．

  - 最小値は `0`, 初期値・最大値は `40` (0.04 秒) です．

- `ハイパスフィルタ` が OFF だとローパスフィルタに（こもった感じの音），ON だとハイパスフィルタ（遠い感じの音）に切り替えます．

- `窓関数` でフィルタの特性になる関数を指定します．

  畳み込み積の計算の核となる関数を指定します．（本来の窓関数の目的とは違う利用先ですが，ユニークな名前が多く選択肢として選びやすいため窓関数という名前にしています．）

  次の 6 つから選びます．

  1.  矩形窓: $\qquad f(x)=1\quad(-1\leqq x \leqq +1)$.
  1.  三角窓: $\qquad f(x)=\min\{ 1+x, 1-x \}$.
  1.  Hann 窓: $\qquad f(x)=0.5+0.5\cos \pi x$.
  1.  Hamming 窓: $\qquad f(x)=0.52+0.46\cos \pi x$.
  1.  Blackman 窓: $\qquad f(x)=0.42+0.50\cos \pi x+0.08\cos 2\pi x$.
  1.  Gauss 窓: $\qquad f(x)=\exp(-\pi x^2)$.

  - 初期値は Hann 窓です．

## TIPS

- デシベル (dB) 単位は $+10 \mathrm{dB}$ 上昇で 10 倍，$+3 \mathrm{dB}$ で約 2 倍と覚えておくと便利です．

- `周波数` を再調整したり，`ハイパスフィルタ` チェックや `窓関数` を変更した場合は，一旦 `補正dB` を `0.00` に再設定すると不意に大音量になってしまうのを防ぐことができます．また調整にはなるべくスライダーを利用して，数値ボックスのドラッグ移動を控えれば最大でも `+20.00` dB (100 倍の音量) に留まってくれます．

- 遅延量の最大想定の約 40 ミリ秒（0.04 秒）は，`周波数` を最小の `-24.00` と想定しての値です．1オクターブ上がるごとに必要最小限の遅延量は半分になるため，本当に最大想定が必要な場面は多くないと思われます．また，`最小遅延` で指定した遅延時間だけ音声オブジェクトを前倒しにして配置すれば，遅延の影響を打ち消すことができます．

- `周波数` を時間変化させない場合は `最小遅延` を `0` に指定すれば，遅延量は計算手順上の必要最小限になってくれます．

## 既知の問題

1.  `周波数` を時間変化させるとノイズが発生することがあります．`最小遅延` を大きくすることで抑制できますが，変化があまり速い場合など抑制しきれない場合があります．


## 改版履歴

- **v1.00** (2024-06-1?)

  - 初版．


## ライセンス・免責事項

このプログラムの利用・改変・再頒布等に関しては CC0 として扱うものとします．


#  Credits

##  aviutl_exedit_sdk

https://github.com/ePi5131/aviutl_exedit_sdk （利用したブランチは[こちら](https://github.com/sigma-axis/aviutl_exedit_sdk/tree/self-use)です．）

---

1条項BSD

Copyright (c) 2022
ePi All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
THIS SOFTWARE IS PROVIDED BY ePi “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ePi BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#  連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://twitter.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social

