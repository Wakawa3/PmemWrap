永続メモリの不適切な永続化処理を検知するデバッグツール

## PMのマウント
まず`df -h`で以下の行があるか確認する．

```sh
/dev/pmem0                         486G   73M  461G   1% /mnt/pmem0
```

あれば"準備"へ．なければ`ndctl list`でPMのmodeが`fsdax`であるか確認する．

```sh
ndctl list

[
  {
    "dev":"namespace1.0",
    "mode":"devdax",
    "map":"mem",
    "size":539016298496,
    "uuid":"3eda0d69-f1aa-44c3-ac29-ae7831759f77",
    "chardev":"dax1.0",
    "align":2097152
  },
  {
    "dev":"namespace0.0",
    "mode":"devdax",
    "map":"mem",
    "size":539016298496,
    "uuid":"fdec9b79-42e0-4b6a-a361-cf2f02a151fe",
    "chardev":"dax0.0",
    "align":2097152
  }
]
```

`fsdax`でなければ以下のコマンドでmodeを変更する。

```sh
sudo ndctl create-namespace --force --mode=fsdax --reconfig=namespace0.0

{
    "dev":"namespace0.0",
    "mode":"fsdax",
    "map":"dev",
    "size":530594136064,
    "uuid":"03733da2-0763-4076-969c-88a3e1eca34f",
    "sector_size":512,
    "align":2097152,
    "blockdev":"pmem0"
}
```

参考
https://docs.redhat.com/ja/documentation/red_hat_enterprise_linux/8/html/managing_storage_devices/creating-a-file-system-dax-namespace-on-an-nvdimm_using-nvdimm-persistent-memory-storage#reconfiguring-an-existing-nvdimm-namespace-to-file-system-dax-mode_creating-a-file-system-dax-namespace-on-an-nvdimm
https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/8/html/managing_storage_devices/using-nvdimm-persistent-memory-storage_managing-storage-devices#creating-a-file-system-dax-namespace-on-an-nvdimm_using-nvdimm-persistent-memory-storage

以下で`/dev/pmem0`を`/mnt/pmem0`にマウントする．

```sh
sudo mkfs.ext4 -b 4096 -E stride=512 -F /dev/pmem0
sudo mount -o dax /dev/pmem0 /mnt/pmem0
```

## 準備
本リポジトリのルート直下にある`.bashrc_pmemwrap`を`~/.bashrc`で読み込むように，以下の3行を`~/.bashrc`に追加する．

```sh
if [ -f ~/path/to/.bashrc_pmemwrap ]; then
    . ~/path/to/.bashrc_pmemwrap
fi
```

`.bashrc`を再読み込みしたら，本リポジトリのルートで以下のコマンドを実行する．

```sh
git submodule update --init --recursive
./install_pmdk.sh
./install_safepm.sh
make
```

## サンプルプログラム

`sample_programs`ディレクトリの`hello_libpmemobjtest4_safepm_pw.c`の挙動をテストする．`sample_programs`ディレクトリで`./objtest4_safepm_pw.sh`を実行してテストする．`objtest4_safepm_pw.sh`の内容は以下．

```sh
gcc hello_libpmemobjtest4_safepm_pw.c -fsanitize=address -fsanitize-recover=all -ggdb -lwrappmem -lpmem -lpmemobj
export PMEMWRAP_ABORT=0
export PMEMWRAP_MEMCPY=RAND_MEMCPY
export PMEMWRAP_WRITECOUNTFILE=ADD
rm -f /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed
./a.out /mnt/pmem0/test4_safepm_pw #この実行は途中でクラッシュするように設定している
${PMEMWRAP_HOME}/bin/PmemWrap_memcpy.out /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed #永続化処理されていない領域を一部のみコピーする
./a.out /mnt/pmem0/test4_safepm_pw #この実行時，PM上のデータを読み込む際に確率的にクラッシュし，不具合があることがわかる
rm -f /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed
```

2回目の`./a.out /mnt/pmem0/test4_safepm_pw`でクラッシュする場合とクラッシュしない場合がある．このサンプルプログラムは適切に永続化処理していないことで再実行時にクラッシュする可能性があり，実際に2回目の実行でクラッシュすることが確認できれば，正常にテストが完了したことになる．

## CCEHのテスト

以下のPMプログラムのテスト
https://github.com/DICL/CCEH

以下を実行する．

```sh
cd test_targets/CCEH/CCEH-PMDK
./testexamples_safepm.sh
```

結果は`CCEH-PMDK`下の`outputs_safepm`ディレクトリに出力される．
