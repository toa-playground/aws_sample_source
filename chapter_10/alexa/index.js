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

const AWS = require('aws-sdk');

// 米国西部 (オレゴン) の AWS Lambda から東京の AWS IoT Core に接続するための設定変更
// リージョンは実際に Greengrass グループを作成したリージョンを指定して下さい
AWS.config.update({
    region: "ap-northeast-1"
});

// endpoint の値は AWS IoT Core の設定画面からコピーして下さい
const iotData = new AWS.IotData({
    endpoint: 'xxxxxxxxxxxxx-ats.iot.ap-northeast-1.amazonaws.com'
});

// 制御対象のデバイス名 (AWS IoT Greengrass に登録したデバイス)
const deviceName = 'shutter';

// Alexa スキルからのリクエストの処理
exports.handler = async function (request, context) {
    if (request.directive.header.namespace === 'Alexa.Discovery' && request.directive.header.name === 'Discover') {
        // 機器発見リクエスト
        return handleDiscovery(request, context);
    }
    else if (request.directive.header.namespace === 'Alexa.ToggleController') {
        // 制御リクエスト
        if (request.directive.header.name === 'TurnOn' || request.directive.header.name === 'TurnOff') {
            return await handleToggleControl(request, context);
        }
    }
    else if (request.directive.header.namespace === 'Alexa' && request.directive.header.name === 'ReportState') {
        // 状態取得リクエスト
        return await handleReportState(request, context);
    }
    else {
        // その他のリクエスト
        console.log("DEBUG: Other Request: " + JSON.stringify(request));
    }
};

// 機器発見リクエストを処理するハンドラー
// ToggleController を持つ「シャッター」というデバイスの情報を返します
function handleDiscovery(request, context) {
    var payload = {
        "endpoints":
        [
            {
                "endpointId": "shutter_id",
                "manufacturerName": "YourCompanyName",
                "friendlyName": "シャッター",
                "description": "シャッター",
                "displayCategories": ["EXTERIOR_BLIND"],
                "cookie": {},
                "capabilities":
                [
                    {
                      "type": "AlexaInterface",
                      "interface": "Alexa",
                      "version": "3"
                    },
                    {
                        "type": "AlexaInterface",
                        "interface": "Alexa.ToggleController",
                        "version": "3",
                        "instance": "Shutter.State",
                        "properties": {
                            "supported": [{
                                "name": "toggleState"
                            }],
                            "retrievable": true
                        },
                        "capabilityResources": {
                            "friendlyNames": [
                                {
                                    "@type": "text",
                                    "value": {
                                        "text": "シャッター",
                                        "locale": "ja-JP"
                                    }
                                }
                            ]
                        },
                        "semantics": {
                            "actionMappings": [
                                {
                                    "@type": "ActionsToDirective",
                                    "actions": ["Alexa.Actions.Close"],
                                    "directive": {
                                        "name": "TurnOn",
                                        "payload": {}
                                    }
                                },
                                {
                                    "@type": "ActionsToDirective",
                                    "actions": ["Alexa.Actions.Open"],
                                    "directive": {
                                        "name": "TurnOff",
                                        "payload": {}
                                    }
                                }
                            ],
                            "stateMappings": [
                                {
                                    "@type": "StatesToValue",
                                    "states": ["Alexa.States.Closed"],
                                    "value": "ON"
                                },
                                {
                                    "@type": "StatesToValue",
                                    "states": ["Alexa.States.Open"],
                                    "value": "OFF"
                                }  
                            ]
                        }
                    }
                ]
            }
        ]
    };
    var header = request.directive.header;
    header.name = "Discover.Response";
    return { event: { header: header, payload: payload } };
}

// 制御リクエストを処理するハンドラー
async function handleToggleControl(request, context) {
    var requestMethod = request.directive.header.name;
    var responseHeader = request.directive.header;
    responseHeader.namespace = "Alexa";
    responseHeader.name = "Response";
    responseHeader.messageId = responseHeader.messageId + "-R";
    var requestToken = request.directive.endpoint.scope.token;

    // ToggleController への TurnOn / TurnOff をデバイスシャドウの is_open の false / true として書き込む
    var isOpen = (requestMethod === "TurnOff");
    const payload = {
      state: {
        desired: {
          is_open: isOpen
        }
      }
    };
    const result = await iotData.updateThingShadow({
      payload: JSON.stringify(payload),
      thingName: deviceName
    }).promise();

    var contextResult = {
        "properties": [{
            "namespace": "Alexa.ToggleController",
            "name": "toggleState",
            "instance": "Shutter.State",
            "value": isOpen ? "OFF" : "ON",
            "timeOfSample": new Date().toISOString(),
            "uncertaintyInMilliseconds": 50
        }]
    };
    var response = {
        context: contextResult,
        event: {
            header: responseHeader,
            endpoint: {
                scope: {
                    type: "BearerToken",
                    token: requestToken
                },
                endpointId: "shutter_id"
            },
            payload: {}
        }
    };
    return response;
}

// 状態取得リクエストを処理するハンドラー
async function handleReportState(request, context) {
    var responseHeader = request.directive.header;
    responseHeader.namespace = "Alexa";
    responseHeader.name = "StateReport";
    responseHeader.messageId = responseHeader.messageId + "-R";
    var requestToken = request.directive.endpoint.scope.token;

    // デバイスシャドウの状態を取得し、is_open を toggleState にマッピングして返す
    const result = await iotData.getThingShadow({thingName: deviceName}).promise();
    const shadow = JSON.parse(result.payload);
    var contextResult = {
        "properties": [{
            "namespace": "Alexa.ToggleController",
            "instance": "Shutter.State",
            "name": "toggleState",
            "value": shadow.state.reported.is_open ? "OFF" : "ON",
            "timeOfSample": new Date().toISOString(),
            "uncertaintyInMilliseconds": 50
        }]
    };
    var response = {
        context: contextResult,
        event: {
            header: responseHeader,
            endpoint: {
                scope: {
                    type: "BearerToken",
                    token: requestToken
                },
                endpointId: "shutter_id"
            },
            payload: {}
        }
    };
    return response;
}
