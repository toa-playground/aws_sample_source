/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

const greengrass = require('aws-greengrass-core-sdk');
const EL = require('echonet-lite');

// AWS IoT Core にアクセスするためのクラス
const iot = new greengrass.IotData();

// 自分自身に関する定義
const myObject = '05ff01'; // オブジェクト種別はコントローラー

// 制御対象とする ECHONET Lite デバイスに関する定義
const deviceName = 'shutter'; // デバイスシャドウを定義したデバイスの名前
const deviceAddress = '192.168.1.100'; // IP アドレス (実際に発見したアドレスに置き換えて下さい)
const deviceObject = '026001'; // オブジェクト種別はブラインド (実際に発見したオブジェクトに置き換えて下さい)

const propertyOpenState = 0xe0;
const valueOpen = 0x41;
const valueClose = 0x42;

// デバイスシャドウの変更通知が行われる MQTT トピックの正規表現パターン
const pattern_updateDelta = /^\$aws\/things\/(?<thingName>.*)\/shadow\/update\/delta$/

// MQTT メッセージを受信したら起動されるハンドラー
exports.handler = async (event, context) => {
  // メッセージを受信したトピック (subject)
  const subject = context.clientContext.Custom.subject;

  // トピックがデバイスシャドウの変更通知かどうかを正規表現で判定する
  let result = subject.match(pattern_updateDelta);
  if (result && result.groups.thingName === deviceName) {
    // 変更通知だった場合は変更内容を取得
    const isOpen = event.state.is_open;

    // 変更内容を制御対象の ECHONET Lite デバイスに送信
    // 開閉状態 (propertyOpenState) を「開 (valueOpen)」または「閉 (valueClose)」に変更する
    const value = [isOpen ? valueOpen : valueClose];
    EL.sendOPC1(deviceAddress, EL.toHexArray(myObject), EL.toHexArray(deviceObject), EL.SETI, propertyOpenState, value);
  }
};

// echonet-lite モジュールの初期化
// ECHONET Lite パケットを受信した際の処理をここで実装
var elsocket = EL.initialize([myObject], async (rinfo, els, error) => {
  // 受信したパケットの種別を調査
  switch(els.ESV) {
  case EL.GET_RES: // GET リクエストへのレスポンス
    if (els.DEOJ === myObject) {
      if (els.SEOJ === deviceObject) {
        // 送信元が制御対象デバイス、かつ宛先が自分自身なら、受信した内容をデバイスシャドウに送信
        const value = els.DETAILs[EL.toHexString(propertyOpenState)];
        if (value) {
          const isOpen = (value === EL.toHexString(valueOpen));
          await updateDeviceShadow(deviceName, isOpen);
        }
      }
    }
    break;

  case EL.INF: // プロパティ変更通知
    if (els.SEOJ === deviceObject) {
      // 送信元が制御対象デバイスなら、内容をデバイスシャドウに送信
      // プロパティ変更通知は不特定多数へのブロードキャストであるため、宛先の判定は不要
      const value = els.DETAILs[EL.toHexString(propertyOpenState)];
      if (value) {
        const isOpen = (value === EL.toHexString(valueOpen));
        await updateDeviceShadow(deviceName, isOpen);
      }
    }
    break;

  default:
    break;
  }
});

// デバイスシャドウの更新を行う関数
const updateDeviceShadow = (thingName, isOpen) => {
  return new Promise((resolve, reject) => {
    // 更新内容の JSON オブジェクトを作成
    const payload = {
      state: {
        reported: {
          is_open: isOpen
        },
        desired: null
      }
    };
    // JSON ドキュメントを送信してデバイスシャドウを更新
    iot.updateThingShadow({
      payload: JSON.stringify(payload),
      thingName: thingName
    }, (err, data) => {
      if (err) {
        reject(err);
      } else {
        resolve(data);
      }
    });
  });
}

// 初回起動時、1回だけ ECHONET Lite 機器から開閉状態 (propertyOpenState) を取得する
// 最新の状態は更新通知で受け取れるが、初回起動時は情報がないため能動的に取得
setTimeout(() => {
  EL.sendOPC1(deviceAddress, EL.toHexArray(myObject), EL.toHexArray(deviceObject), EL.GET, propertyOpenState, []);
}, 3000);
