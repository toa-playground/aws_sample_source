#########################################
# Sample for Alexa Camera Skill
# This sample use Amazon Kinesis Video Stream with WebRTC for the Camera Skill(Alexa.RTCSessionController)
# https://developer.amazon.com/ja-JP/docs/alexa/smarthome/build-smart-home-camera-skills.html
#########################################
import base64
import boto3
import json
import logging
import uuid
import os

#########################################
# Set up following Lambda Environment Variable
#########################################
# Kinesis Video Stream WebRTC Channel ARN
CHANNEL_ARN = os.environ.get("CHANNEL_ARN", "")
# Frendly Name for Alexa Smart Home Device
FRIENDLY_NAME = os.environ.get("FRIENDLY_NAME", "")
KVS_REGION = CHANNEL_ARN.split(":")[3]

logger = logging.getLogger()
logger.setLevel(logging.INFO)


#########################################
# Generate uuid
#########################################
def get_uuid():
    return str(uuid.uuid4())


#########################################
# Handle Alexa Skill Request
# https://developer.amazon.com/ja-JP/docs/alexa/smarthome/understand-the-smart-home-skill-api.html
#########################################
def lambda_handler(request, context):
    logger.info("Request ->")
    logger.info(json.dumps(request, indent=2, sort_keys=True))

    response = None
    if request["directive"]["header"]["namespace"] == "Alexa.Discovery" and \
       request["directive"]["header"]["name"] == "Discover":
        response = handle_discovery(request)
    elif request["directive"]["header"]["namespace"] == "Alexa.Authorization" and \
       request["directive"]["header"]["name"] == "AcceptGrant":
        response = handle_accept_grant(request)
    elif request["directive"]["header"]["namespace"] == "Alexa.RTCSessionController":
        if request["directive"]["header"]["name"] == "InitiateSessionWithOffer":
            response = handle_initiate_session(request)
        elif request["directive"]["header"]["name"] == "SessionDisconnected":
            response = handle_session_disconnected(request)

    if response is None:
        raise Exception("Interface is not implemented")

    logger.info("Response ->")
    logger.info(json.dumps(response, indent=2, sort_keys=True))

    return response


#########################################
# Handle Discovery Request and return Discovery response
# https://developer.amazon.com/ja-JP/docs/alexa/device-apis/alexa-discovery.html#discover
#########################################
def handle_discovery(request):

    response = {
        "event": {
            "header": {
                "namespace": "Alexa.Discovery",
                "name": "Discover.Response",
                "payloadVersion": "3",
                "messageId": get_uuid(),
            },
            "payload": {
                "endpoints": [
                    {
                        "endpointId": "kvs_webrtc_RPi",
                        "manufacturerName": "example.com",
                        "description": "test kvs webrtc camera",
                        "friendlyName": FRIENDLY_NAME,
                        "displayCategories": ["CAMERA"],
                        "cookie": {},
                        "capabilities": [
                            {
                                "type": "AlexaInterface",
                                "interface": "Alexa.RTCSessionController",
                                "version": "3",
                                "configuration": {"isFullDuplexAudioSupported": True},
                            },
                            {
                                "type": "AlexaInterface",
                                "interface": "Alexa",
                                "version": "3",
                            },
                        ],
                    }
                ]
            },
        }
    }

    return response

#########################################
# Handle AcceptGrant request and return dummy responce
# https://developer.amazon.com/ja-JP/docs/alexa/device-apis/alexa-authorization.html
#########################################
def handle_accept_grant(request):

    response = {
      "event": {
        "header": {
          "namespace": "Alexa.Authorization",
          "name": "AcceptGrant.Response",
          "messageId": get_uuid(),
          "payloadVersion": "3"
        },
        "payload": {}
      }
    }

    return response

#########################################
# Handle RTCSession Request and return SDP Answer response
# https://developer.amazon.com/ja-JP/docs/alexa/device-apis/alexa-rtcsessioncontroller.html
#########################################
def handle_initiate_session(request):
    kv_client = boto3.client("kinesisvideo", region_name=KVS_REGION)
    res = kv_client.get_signaling_channel_endpoint(
        ChannelARN=CHANNEL_ARN,
        SingleMasterChannelEndpointConfiguration={
            "Protocols": ["HTTPS",],
            "Role": "VIEWER",
        },
    )

    kvs_client = boto3.client(
        "kinesis-video-signaling",
        region_name=KVS_REGION,
        endpoint_url=res["ResourceEndpointList"][0]["ResourceEndpoint"],
    )
    offer = {"type": "offer", "sdp": request["directive"]["payload"]["offer"]["value"]}

    answer_res = kvs_client.send_alexa_offer_to_master(
        ChannelARN=CHANNEL_ARN,
        SenderClientId="ProducerMaster",
        MessagePayload=base64.b64encode(json.dumps(offer).encode()).decode(),
    )
    logger.debug(answer_res)

    if "Answer" in answer_res:
        sdp_answer = json.loads(base64.b64decode(answer_res["Answer"]).decode("utf-8"))[
            "sdp"
        ]
        logger.debug(sdp_answer)
    else:
        sdp_answer = None
        logger.error("SDP Answer does not exist!")

    response = {
        "event": {
            "header": {
                "namespace": "Alexa.RTCSessionController",
                "name": "AnswerGeneratedForSession",
                "messageId": get_uuid(),
                "correlationToken": request["directive"]["header"]["correlationToken"],
                "payloadVersion": "3",
            },
            "endpoint": {
                "scope": {
                    "type": "BearerToken",
                    "token": request["directive"]["endpoint"]["scope"]["token"],
                },
                "endpointId": request["directive"]["endpoint"]["endpointId"],
            },
            "payload": {"answer": {"format": "SDP", "value": sdp_answer}},
        }
    }
    return response


#########################################
# Handle SessionDisconnected Request and return SessionDisconnected response
# https://developer.amazon.com/ja-JP/docs/alexa/device-apis/alexa-rtcsessioncontroller.html
#########################################
def handle_session_disconnected(request):
    response = {
        "event": {
            "header": {
                "namespace": "Alexa.RTCSessionController",
                "name": "SessionDisconnected",
                "messageId": get_uuid(),
                "correlationToken": request["directive"]["header"]["correlationToken"],
                "payloadVersion": "3",
            },
            "endpoint": {
                "scope": {
                    "type": "BearerToken",
                    "token": request["directive"]["endpoint"]["scope"]["token"],
                },
                "endpointId": request["directive"]["endpoint"]["endpointId"],
            },
            "payload": {"sessionId": request["directive"]["payload"]["sessionId"]},
        }
    }

    return response
