永続メモリの不適切な永続化処理を検知するデバッグツール

## はじめに

はじめに'ndctl list'でPMのmodeを確認する.

'''sh
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
'''