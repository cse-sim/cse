require_relative 'test_helper'
require 'coverage_check'
require 'set'

class CoverageCheckTest < Minitest::Test
  include CoverageCheck
  def setup
    @coolplant_path = File.expand_path('../../src/records/coolplant.md', __FILE__)
    @airhandler_path = File.expand_path('../../src/records/airhandler.md', __FILE__)
  end
  def test_MdHead
    line = "3"
    expected = nil
    actual = MdHeader[line]
    assert_equal(expected, actual)
    #
    line = "# A Header"
    expected = "A Header"
    actual = MdHeader[line]
    assert_equal(expected, actual)
  end
  def test_DataItemHeader
    line = "3"
    expected = nil
    actual = DataItemHeader[line]
    assert_equal(expected, actual)
    #
    line = "**sfanCurvePy=$k_0$, $k_1$, $k_2$, $k_3$, $x_0$**"
    expected = "sfanCurvePy"
    actual = DataItemHeader[line]
    assert_equal(expected, actual)
    #
    line = "  **Units** **Legal Range** **Default** **Required** **Variability**"
    expected = nil
    actual = DataItemHeader[line]
    assert_equal(expected, actual)
    #
    line = "**ahTsSp=*float or choice***"
    expected = "ahTsSp"
    actual = DataItemHeader[line]
    assert_equal(expected, actual)
    #
    line = "*AhTsRaMn* and *ahTsRaMx* are used when *ahTsSp* is RA."
    expected = nil
    actual = DataItemHeader[line]
    assert_equal(expected, actual)
  end
  def test_ParseRecordDocument
    content = File.read(@coolplant_path)
    expected = {
      "COOLPLANT" => Set.new([
        "coolplantName",
        "cpSched",
        "cpTsSp",
        "cpPipeLossF",
        "cpTowerplant",
        "cpStage1",
        "cpStage2 through cpStage7 same",
        "endCoolplant",
      ])
    }
    actual = ParseRecordDocument[content]
    assert_equal(expected, actual)
  end
  def test_ReadRecordDocument
    expected = {
      "COOLPLANT" => Set.new([
        "coolplantName",
        "cpSched",
        "cpTsSp",
        "cpPipeLossF",
        "cpTowerplant",
        "cpStage1",
        "cpStage2 through cpStage7 same",
        "endCoolplant",
      ])
    }
    actual = ReadRecordDocument[@coolplant_path]
    assert_equal(expected, actual)
  end
  def test_ReadAllRecordDocuments
    actual = ReadAllRecordDocuments[[@airhandler_path, @coolplant_path]]
    expected = {
      "AIRHANDLER" => Set.new([
        "ahName",
        "ahSched"
      ]),
      "AIRHANDLER Supply Air Temperature Controller" => Set.new([
        "ahTsSp",
        "ahFanCycles",
        "ahTsMn",
        "ahTsMx",
        "ahWzCzns",
        "ahCzCzns",
        "ahCtu",
        "ahTsRaMx",
        "ahTsRaMx",
      ]),
      "AIRHANDLER Supply fan" => Set.new([
        "sfanType",
        "sfanVfDs",
        "sfanVfMxF",
        "sfanPress",
        "sfanElecPwr",
        "sfanEff",
        "sfanShaftBhp",
        "sfanCurvePy",
        "sfanMotEff",
        "sfanMotPos",
        "sfanMtr",
      ]),
      "AIRHANDLER Return/Relief fan" => Set.new([
        "rfanType",
        "rfanVfDs",
        "rfanVfMxF",
        "rfanPress",
        "rfanElecPwr",
        "rfanEff",
        "rfanShaftBhp",
        "rfanCurvePy",
        "rfanMotEff",
        "rfanMotPos",
        "rfanMtr",
      ]),
      "AIRHANDLER Heating coil/Modeling Furnaces" => Set.new([
        "ahhcType",
        "ahhcSched",
        "ahhcCapTRat",
        "ahhcMtr",
        "ahhcHeatplant",
        "ahhcEirR",
        "ahhcEffR",
        "ahhcPyEi",
        "ahhcStackEffect",
        "ahpCap17",
        "ahpCap35",
        "ahpCap47",
        "ahpFd35Df",
        "ahpCapIa",
        "ahpSupRh",
        "ahpTFrMn",
        "ahpTFrMx",
        "ahpTFrPk",
        "ahpDfrFMn",
        "ahpDfrFMx",
        "ahpDfrCap",
        "ahpDfrRh",
        "ahpTOff",
        "ahpTOn",
        "ahpIn17",
        "ahpIn47",
        "ahpInIa",
        "ahpCd",
        "ahhcAuxOn",
        "ahhcAuxOff",
        "ahhcAuxFullOff",
        "ahhcAuxOnAtAll",
        "ahhcAuxOnMtr",
        "ahhcAuxOffMtr",
        "ahhcAuxFullOffMtr",
        "ahhcAuxOnAtAllMtr",
      ]),
      "AIRHANDLER Cooling coil" => Set.new([
        "ahccType",
        "ahccSched",
        "ahccCapTRat",
        "ahccCapSRat",
        "ahccMtr",
        "ahccMinTEvap",
        "ahccK1",
        "ahccBypass",
        "ahccEirR",
        "ahccMinUnldPlr",
        "ahccMinFsldPlr",
        "pydxCaptT",
        "pydxCaptF",
        "pydxEirT",
        "pydxEirUl",
        "ahccCoolplant",
        "ahccGpmDs",
        "ahccNtuoDs",
        "ahccNtuiDs",
        "ahccDsTDbEn",
        "ahccDsTWbEn",
        "ahccDsTDbCnd",
        "ahccVfR",
        "ahccAuxOn",
        "ahccAuxOff",
        "ahccAuxFullOff",
        "ahccAuxOnAtAll",
        "ahccAuxOnMtr",
        "ahccAuxOffMtr",
        "ahccAuxFullOffMtr",
        "ahccAuxOnAtAllMtr",
      ]),
      "AIRHANDLER Outside Air" => Set.new([
        "oaMnCtrl",
        "oaVfDsMn",
        "oaMnFrac",
        "oaEcoType",
        "oaLimT",
        "oaLimE",
        "oaOaLeak",
        "oaRaLeak",
      ]),
      "AIRHANDLER Leaks and Losses" => Set.new([
        "ahSOLeak",
        "ahROLeak",
        "ahSOLoss",
        "ahROLoss",
      ]),
      "AIRHANDLER Crankcase Heater" => Set.new([
        "cchCM",
        "cchPMx",
        "cchPMn",
        "cchTMx",
        "cchTMn",
        "cchDT",
        "cchTOn",
        "cchTOff",
        "cchMtr",
        "endAirHandler",
      ]),
      "COOLPLANT" => Set.new([
        "coolplantName",
        "cpSched",
        "cpTsSp",
        "cpPipeLossF",
        "cpTowerplant",
        "cpStage1",
        "cpStage2 through cpStage7 same",
        "endCoolplant",
      ])
    }
    expected.keys.each do |k|
      assert(actual.include?(k), "actual doesn't include #{k}")
      assert_equal(expected[k], actual[k])
    end
    actual.keys.each do |k|
      assert(expected.include?(k), "expected doesn't include #{k}")
      assert_equal(expected[k], actual[k])
    end
    assert_equal(expected, actual)
  end
  def test_ParseCulList
    content = <<DOC

CSE 0.816 for Win32 console
Command line: -c

Top
   doAutoSize
   doMainSim


material    Parent: Top
   matThk
   matCond

DOC
    expected = {
      "Top" => Set.new(["doAutoSize", "doMainSim"]),
      "material" => Set.new(["matThk", "matCond"])
    }
    actual = ParseCulList[content]
    assert_equal(expected, actual)
  end
end
