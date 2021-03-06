/**
 *    Copyright (C) 2016 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/s/request_types/merge_chunk_request_type.h"

#include "mongo/unittest/unittest.h"

namespace mongo {

namespace {

using unittest::assertGet;

TEST(MergeChunkRequest, BasicValidConfigCommand) {
    auto request = assertGet(MergeChunkRequest::parseFromConfigCommand(
        BSON("_configsvrMergeChunk"
             << "TestDB.TestColl"
             << "collEpoch"
             << OID("7fffffff0000000000000001")
             << "chunkBoundaries"
             << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 5) << BSON("a" << 10)))));
    ASSERT_EQ(NamespaceString("TestDB", "TestColl"), request.getNamespace());
    ASSERT_EQ(OID("7fffffff0000000000000001"), request.getEpoch());
    ASSERT_EQ(BSON("a" << 1), request.getChunkBoundaries().at(0));
    ASSERT_EQ(BSON("a" << 5), request.getChunkBoundaries().at(1));
    ASSERT_EQ(BSON("a" << 10), request.getChunkBoundaries().at(2));
}

TEST(MergeChunkRequest, ConfigCommandtoBSON) {
    BSONObj serializedRequest =
        BSON("_configsvrMergeChunk"
             << "TestDB.TestColl"
             << "collEpoch"
             << OID("7fffffff0000000000000001")
             << "chunkBoundaries"
             << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 5) << BSON("a" << 10)));
    BSONObj writeConcernObj = BSON("writeConcern" << BSON("w"
                                                          << "majority"));

    BSONObjBuilder cmdBuilder;
    {
        cmdBuilder.appendElements(serializedRequest);
        cmdBuilder.appendElements(writeConcernObj);
    }

    auto request = assertGet(MergeChunkRequest::parseFromConfigCommand(serializedRequest));
    auto requestToBSON = request.toConfigCommandBSON(writeConcernObj);

    ASSERT_EQ(cmdBuilder.obj(), requestToBSON);
}

TEST(MergeChunkRequest, MissingNameSpaceErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(
        BSON("collEpoch" << OID("7fffffff0000000000000001") << "chunkBoundaries"
                         << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 5) << BSON("a" << 10))));
    ASSERT_EQ(ErrorCodes::NoSuchKey, request.getStatus());
}

TEST(MergeChunkRequest, MissingCollEpochErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(
        BSON("_configsvrMergeChunk"
             << "TestDB.TestColl"
             << "chunkBoundaries"
             << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 5) << BSON("a" << 10))));
    ASSERT_EQ(ErrorCodes::NoSuchKey, request.getStatus());
}

TEST(MergeChunkRequest, MissingChunkBoundariesErrors) {
    auto request =
        MergeChunkRequest::parseFromConfigCommand(BSON("_configsvrMergeChunk"
                                                       << "TestDB.TestColl"
                                                       << "collEpoch"
                                                       << OID("7fffffff0000000000000001")));
    ASSERT_EQ(ErrorCodes::NoSuchKey, request.getStatus());
}

TEST(MergeChunkRequest, WrongNamespaceTypeErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(BSON(
        "_configsvrMergeChunk" << 1234 << "collEpoch" << OID("7fffffff0000000000000001")
                               << "chunkBoundaries"
                               << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 5) << BSON("a" << 10))));
    ASSERT_EQ(ErrorCodes::TypeMismatch, request.getStatus());
}

TEST(MergeChunkRequest, WrongCollEpochTypeErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(
        BSON("_configsvrMergeChunk"
             << "TestDB.TestColl"
             << "collEpoch"
             << 1234
             << "chunkBoundaries"
             << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 5) << BSON("a" << 10))));
    ASSERT_EQ(ErrorCodes::TypeMismatch, request.getStatus());
}

TEST(MergeChunkRequest, WrongChunkBoundariesTypeErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(BSON("_configsvrMergeChunk"
                                                                  << "TestDB.TestColl"
                                                                  << "collEpoch"
                                                                  << OID("7fffffff0000000000000001")
                                                                  << "chunkBoundaries"
                                                                  << 1234));
    ASSERT_EQ(ErrorCodes::TypeMismatch, request.getStatus());
}

TEST(MergeChunkRequest, InvalidNamespaceErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(
        BSON("_configsvrMergeChunk"
             << ""
             << "collEpoch"
             << OID("7fffffff0000000000000001")
             << "chunkBoundaries"
             << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 5) << BSON("a" << 10))));
    ASSERT_EQ(ErrorCodes::InvalidNamespace, request.getStatus());
}

TEST(MergeChunkRequest, EmptyChunkBoundariesErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(BSON("_configsvrMergeChunk"
                                                                  << "TestDB.TestColl"
                                                                  << "collEpoch"
                                                                  << OID("7fffffff0000000000000001")
                                                                  << "chunkBoundaries"
                                                                  << BSONArray()));
    ASSERT_EQ(ErrorCodes::InvalidOptions, request.getStatus());
}

TEST(MergeChunkRequest, TooFewChunkBoundariesErrors) {
    auto request = MergeChunkRequest::parseFromConfigCommand(
        BSON("_configsvrMergeChunk"
             << "TestDB.TestColl"
             << "collEpoch"
             << OID("7fffffff0000000000000001")
             << "chunkBoundaries"
             << BSON_ARRAY(BSON("a" << 1) << BSON("a" << 10))));
    ASSERT_EQ(ErrorCodes::InvalidOptions, request.getStatus());
}
}

}  // namespace mongo
