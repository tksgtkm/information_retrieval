#ifndef CONFIG_H
#define CONFIG_H

// 0か1を設定値とする。1にするとallocationのデバッグを行う。
#define ALLOC_DEBUG 0

/*
インデックスのoffsetをint32bitか64bitを選択する
(基本は64itでよい)
*/
#define INDEX_OFFSET_BITS 64

/*
トークンの最大長
*/
#define MAX_TOKEN_LENGTH 19

/*
異なるタームのポスティングはディスク上インデックスのブロック内にグルーピングされる。
ブロックごとにメモリ内に記述子がある。
これはディスク上のインデックスのブロックのターゲットサイズを指す。
*/
#define BYTES_PER_INDEX_BLOCK 65536

/*
同じタームにあるすべてのポスティングはセグメントに配置される。
すべてのタームを同時にRAMにロードするのに十分なメモリがない場合は
メモリに3つのセグメントを確保する。
このときに確保するセグメントのサイズを設定する。
この値の大きさはパフォーマンスとメモリ消費のトレードオフの関係になる。
*/
#define TARGET_SEGMENT_SIZE 32768

#define MIN_SEGMENT_SIZE (int)(0.65 * TARGET_SEGMENT_SIZE)
#define MAX_SEGMENT_SIZE (int)(0.65 * TARGET_SEGMENT_SIZE)

#endif