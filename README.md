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
