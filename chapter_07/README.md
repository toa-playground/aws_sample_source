# サブモジュールの取得

このサンプルソースでは、以下の2つのリポジトリを必要とします。

- https://github.com/aws/amazon-freertos.git
- https://github.com/teuteuguy/afr-m5stickc-bsp.git

このファイルと同じディレクトリで、以下のコマンドを実行して取り込んでください。

```
cd src/chapter_07
git clone https://github.com/teuteuguy/afr-m5stickc-bsp.git -b aa4ecc1f components/afr-m5stickc-bsp
cd components/afr-m5stickc-bsp/
git checkout aa4ecc1f

cd ../../
git clone --recursive https://github.com/aws/amazon-freertos.git -b 202012.00 amazon-freertos
```