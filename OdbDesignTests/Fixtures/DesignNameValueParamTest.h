#pragma once

#include "gtest/gtest.h"
#include <filesystem>
#include "OdbDesign.h"
#include <memory>
#include <string>
#include "FileArchiveLoadFixture.h"

namespace Odb::Test::Fixtures
{
	class DesignNameValueParamTest : public FileArchiveLoadFixture,
									 public testing::WithParamInterface<std::string>
	{

	};
}