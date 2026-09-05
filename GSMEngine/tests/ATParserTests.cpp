#include "ATMessageFramer.hpp"
#include "ATParser.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(ATParserTests, ParsesValidSmsHeader)
{
    const auto header = AT::parseSmsHeader("+CMT: \"+48123456789\",,\"26/08/11,12:34:56+08\"\r\n");

    ASSERT_TRUE(header.has_value());
    EXPECT_EQ(header->number, "+48123456789");
    EXPECT_EQ(header->dateAndTime, "26/08/11,12:34:56+08");
}

TEST(ATParserTests, RejectsInvalidSmsHeader)
{
    const auto header = AT::parseSmsHeader("+CMT: broken header\r\n");

    EXPECT_FALSE(header.has_value());
}

TEST(ATParserTests, ParsesEmptySmsBody)
{
    const auto body = AT::parseSmsBody("\r\n");

    EXPECT_TRUE(body.empty());
}

TEST(ATParserTests, ParsesClipNumber)
{
    const auto number = AT::parseClipNumber("+CLIP: \"+48123456789\",145,,,\"\",0\r\n");

    ASSERT_TRUE(number.has_value());
    EXPECT_EQ(*number, "+48123456789");
}

TEST(ATMessageFramerTests, ReturnsMultipleMessagesFromOneRead)
{
    AT::ATMessageFramer framer(4096);

    const auto result = framer.push("OK\r\nRING\r\n");

    EXPECT_FALSE(result.overflow);
    ASSERT_EQ(result.messages.size(), 2);
    EXPECT_EQ(result.messages[0], "OK\r\n");
    EXPECT_EQ(result.messages[1], "RING\r\n");
}

TEST(ATMessageFramerTests, ReturnsMessageSplitBetweenReads)
{
    AT::ATMessageFramer framer(4096);

    const auto firstRead = framer.push("+CMT: \"+48123456789\"");
    const auto secondRead = framer.push(",,\"26/08/11,12:34:56+08\"\r\n");

    EXPECT_TRUE(firstRead.messages.empty());
    EXPECT_FALSE(secondRead.overflow);
    ASSERT_EQ(secondRead.messages.size(), 1);
    EXPECT_EQ(secondRead.messages[0], "+CMT: \"+48123456789\",,\"26/08/11,12:34:56+08\"\r\n");
}

TEST(ATMessageFramerTests, FlushesSmsInputPromptWithoutLineDelimiter)
{
    AT::ATMessageFramer framer(4096);

    const auto promptRead = framer.push(">");
    const auto prompt = framer.flushPending();

    EXPECT_TRUE(promptRead.messages.empty());
    ASSERT_TRUE(prompt.has_value());
    EXPECT_EQ(*prompt, ">");
}
