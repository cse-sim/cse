require_relative 'test_helper'
require 'coverage_check'
require 'set'

class CoverageCheckTest < Minitest::Test
  include CoverageCheck
  def setup
    @coolplant_path = File.expand_path('../resources/coolplant.md', __FILE__)
    @airhandler_path = File.expand_path('../resources/airhandler.md', __FILE__)
    @cullist_path = File.expand_path('../resources/cullist-snippet.txt', __FILE__)
    @cullist_full_path = File.expand_path('../resources/cullist.txt', __FILE__)
    @all_records_path = Dir[File.expand_path('../resources/*.md', __FILE__)]
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
    content = File.read(@cullist_path)
    expected = {
      "Top" => Set.new(["doAutoSize", "doMainSim"]),
      "material" => Set.new(["matThk", "matCond"])
    }
    actual = ParseCulList[content]
    assert_equal(expected, actual)
  end
  def test_ReadCulList
    expected = {
      "Top" => Set.new(["doAutoSize", "doMainSim"]),
      "material" => Set.new(["matThk", "matCond"])
    }
    actual = ReadCulList[@cullist_path]
    assert_equal(expected, actual)
  end
  def test_SetDifferences
    #
    s1 = Set.new(["A", "B", "C"])
    s2 = Set.new(["B", "C", "D"])
    actual = SetDifferences[s1, s2, false]
    expected = {
      :in_1st_not_2nd => Set.new(["A"]),
      :in_2nd_not_1st => Set.new(["D"])
    }
    assert_equal(expected, actual)
    #
    s1 = Set.new(["1","2","3"])
    s2 = Set.new(["1","2","3"])
    actual = SetDifferences[s1, s2, false]
    expected = nil
    assert_equal(expected, actual)
    #
    s1 = Set.new(["A","B","C"])
    s2 = Set.new(["a","b","c"])
    actual = SetDifferences[s1, s2, false]
    expected = nil
    assert_equal(expected, actual)
    #
    s1 = Set.new(["A","B","C"])
    s2 = Set.new(["a","b","c"])
    actual = SetDifferences[s1, s2, true]
    expected = {
      :in_1st_not_2nd => Set.new(["A","B","C"]),
      :in_2nd_not_1st => Set.new(["a","b","c"])
    }
    assert_equal(expected, actual)
  end
  def test_AdjustMap
    ris = {
      "A" => Set.new(["a"]),
      "A B" => Set.new(["b"]),
      "A B C" => Set.new(["c"])
    }
    expected = {
      "A" => Set.new(["a", "b", "c"])
    }
    actual = AdjustMap[ris]
    assert_equal(expected, actual)
  end
  def test_DiffsToString
    m = {
      :a=>Set.new(["a"]),
      :b=>Set.new(["b"])
    }
    expected = "Stuff:\nin a but not in b:\n- a\nin b but not in a:\n- b\n"
    actual = DiffsToString[m,"Stuff",:a,:b,"a","b"]
    assert_equal(expected, actual)
  end
  def test_RecordInputSetDifferencesToString
    diffs = {
      :records_in_1st_not_2nd => nil,
      :records_in_2nd_not_1st => nil,
      :field_set_differences => {
        "coolsys" => {
          :in_2nd_not_1st => Set.new(["csName"]),
          :in_1st_not_2nd => nil
        }
      }
    }
    assert("".class == RecordInputSetDifferencesToString[diffs].class)
  end
  def test_RecordInputSetDifferences
    ris1 = {
      "Top" => Set.new(["a", "b", "c"]),
      "Bottom" => Set.new(["a", "b", "c"])
    }
    ris2 = {
      "Top" => Set.new(["a", "B", "c"]),
      "Bottom" => Set.new(["a", "b", "C"])
    }
    actual = RecordInputSetDifferences[ris1, ris2, false]
    expected = nil
    assert_equal(expected, actual)
    #
    actual = RecordInputSetDifferences[ris1, ris2, true]
    expected = {
      records_in_1st_not_2nd: nil,
      records_in_2nd_not_1st: nil,
      field_set_differences: {
        "Top" => {
          in_1st_not_2nd: Set.new(["b"]),
          in_2nd_not_1st: Set.new(["B"])
        },
        "Bottom" => {
          in_1st_not_2nd: Set.new(["c"]),
          in_2nd_not_1st: Set.new(["C"])
        }
      }
    }
    assert_equal(expected, actual)
    #
    ris1 = {
      "Top" => Set.new(["a", "b", "c"]),
      "Bottom" => Set.new(["a", "b", "c"])
    }
    ris2 = {
      "Top" => Set.new(["a", "b", "c"]),
      "Side" => Set.new(["a", "b", "c"])
    }
    actual = RecordInputSetDifferences[ris1, ris2, true]
    expected = {
      records_in_1st_not_2nd: Set.new(["Bottom"]),
      records_in_2nd_not_1st: Set.new(["Side"])
    }
    assert_equal(expected, actual)
    #
    ris1 = {
      "Top" => Set.new(["b", "c"]),
      "Bottom" => Set.new(["a", "b", "c"])
    }
    ris2 = {
      "Top" => Set.new(["a", "b", "c"]),
      "Bottom" => Set.new(["b", "c", "d"])
    }
    actual = RecordInputSetDifferences[ris1, ris2, true]
    expected = {
      records_in_1st_not_2nd: nil,
      records_in_2nd_not_1st: nil,
      field_set_differences: {
        "Top" => {in_1st_not_2nd: Set.new, in_2nd_not_1st: Set.new(["a"])},
        "Bottom" => {
          in_1st_not_2nd: Set.new(["a"]), in_2nd_not_1st: Set.new(["d"])
        }
      }
    }
    assert_equal(expected, actual)
    ris1 = ReadCulList[@cullist_full_path]
    ris2 = ReadAllRecordDocuments[@all_records_path]
    actual = RecordInputSetDifferences[ris1, ris2, false]
    assert(!actual.nil?)
    ris3 = AdjustMap[ris2]
    actual = RecordInputSetDifferences[ris1, ris3, false]
    assert(!actual.nil?)
  end
end
