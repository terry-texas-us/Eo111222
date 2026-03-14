#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "..\EoDxfEntities.h"
#include "..\EoDxfHatch.h"
#include "..\EoDxfInterface.h"
#include "..\EoDxfRead.h"
#include "..\EoDxfTables.h"
#include "..\EoDxfTextCodec.h"
#include "..\EoDxfWrite.h"

namespace {

class RoundTripFixtureModel final : public EoDxfInterface {
 public:
  void SetWriter(EoDxfWrite* writer) noexcept { m_writer = writer; }

  void addHeader(const EoDxfHeader* header) override {
    if (header != nullptr) { m_header = *header; }
  }

  void addClass(const EoDxfClass& classData) override { m_classes.push_back(classData); }
  void addLinetype(const EoDxfLinetype&) override {}
  void addLayer(const EoDxfLayer& layer) override { m_layers.push_back(layer); }
  void addDimStyle(const EoDxfDimensionStyle&) override {}
  void addVport(const EoDxfVPort&) override {}
  void addTextStyle(const EoDxfTextStyle& textStyle) override { m_textStyles.push_back(textStyle); }
  void addAppId(const EoDxfAppId&) override {}
  void addBlock(const EoDxfBlock&) override {}
  void setBlock(const int) override {}
  void endBlock() override {}
  void addPoint(const EoDxfPoint&) override {}
  void addLine(const EoDxfLine&) override {}
  void addRay(const EoDxfRay&) override {}
  void addXline(const EoDxfXline&) override {}
  void addArc(const EoDxfArc&) override {}
  void addCircle(const EoDxfCircle&) override {}
  void addEllipse(const EoDxfEllipse&) override {}
  void addLWPolyline(const EoDxfLwPolyline&) override {}
  void addPolyline(const EoDxfPolyline&) override {}
  void addSpline(const EoDxfSpline*) override {}
  void addKnot(const EoDxfGraphic&) override {}
  void addInsert(const EoDxfInsert&) override {}
  void addTrace(const EoDxfTrace&) override {}
  void add3dFace(const EoDxf3dFace&) override {}
  void addSolid(const EoDxfSolid&) override {}
  void addMText(const EoDxfMText& mText) override { m_mTexts.push_back(mText); }
  void addText(const EoDxfText& text) override { m_texts.push_back(text); }
  void addDimAlign(const EoDxfAlignedDimension*) override {}
  void addDimLinear(const EoDxfDimLinear*) override {}
  void addDimRadial(const EoDxfRadialDimension*) override {}
  void addDimDiametric(const EoDxfDiametricDimension*) override {}
  void addDimAngular(const EoDxf2LineAngularDimension*) override {}
  void addDimAngular3P(const EoDxf3PointAngularDimension*) override {}
  void addDimOrdinate(const EoDxfOrdinateDimension*) override {}
  void addLeader(const EoDxfLeader*) override {}
  void addMLeader(const EoDxfMLeader*) override {}
  void addHatch(const EoDxfHatch&) override {}
  void addViewport(const EoDxfViewport&) override {}
  void addImage(const EoDxfImage*) override {}
  void linkImage(const EoDxfImageDefinition* imageDefinition) override {
    if (imageDefinition != nullptr) { m_imageDefinitions.push_back(*imageDefinition); }
  }
  void addUnsupportedObject(const EoDxfUnsupportedObject& objectData) override { m_unsupportedObjects.push_back(objectData); }
  void addComment(std::wstring_view comment) override { m_comments.emplace_back(comment); }

  void writeHeader(EoDxfHeader& data) override { data = m_header; }
  void writeClasses() override {
    if (m_writer == nullptr) { return; }
    for (auto& classData : m_classes) {
      m_writer->WriteClass(&classData);
    }
  }
  void writeBlocks() override {}
  void writeBlockRecords() override {}
  void writeEntities() override {
    if (m_writer == nullptr) { return; }
    for (auto& text : m_texts) {
      m_writer->WriteText(&text);
    }
    for (auto& mText : m_mTexts) {
      m_writer->WriteMText(&mText);
    }
  }
  void writeObjects() override {
    if (m_writer == nullptr) { return; }
    for (const auto& imageDefinition : m_imageDefinitions) {
      m_writer->AddImageDefinition(imageDefinition);
    }
  }
  void writeUnsupportedObjects() override {
    if (m_writer == nullptr) { return; }
    for (const auto& objectData : m_unsupportedObjects) {
      m_writer->WriteUnsupportedObject(objectData);
    }
  }
  void writeLTypes() override {}
  void writeLayers() override {
    if (m_writer == nullptr) { return; }
    for (auto& layer : m_layers) {
      m_writer->WriteLayer(&layer);
    }
  }
  void writeTextstyles() override {
    if (m_writer == nullptr) { return; }
    for (auto& textStyle : m_textStyles) {
      m_writer->WriteTextstyle(&textStyle);
    }
  }
  void writeVports() override {}
  void writeDimstyles() override {}
  void writeAppId() override {}

  [[nodiscard]] const EoDxfHeader& Header() const noexcept { return m_header; }
  [[nodiscard]] const std::vector<EoDxfClass>& Classes() const noexcept { return m_classes; }
  [[nodiscard]] const std::vector<EoDxfImageDefinition>& ImageDefinitions() const noexcept { return m_imageDefinitions; }
  [[nodiscard]] const std::vector<EoDxfUnsupportedObject>& UnsupportedObjects() const noexcept { return m_unsupportedObjects; }
  [[nodiscard]] const std::vector<EoDxfLayer>& Layers() const noexcept { return m_layers; }
  [[nodiscard]] const std::vector<EoDxfTextStyle>& TextStyles() const noexcept { return m_textStyles; }
  [[nodiscard]] const std::vector<EoDxfText>& Texts() const noexcept { return m_texts; }
  [[nodiscard]] const std::vector<EoDxfMText>& MTexts() const noexcept { return m_mTexts; }

 private:
  EoDxfWrite* m_writer{};
  EoDxfHeader m_header;
  std::vector<EoDxfClass> m_classes;
  std::vector<EoDxfImageDefinition> m_imageDefinitions;
  std::vector<EoDxfUnsupportedObject> m_unsupportedObjects;
  std::vector<EoDxfLayer> m_layers;
  std::vector<EoDxfTextStyle> m_textStyles;
  std::vector<EoDxfText> m_texts;
  std::vector<EoDxfMText> m_mTexts;
  std::vector<std::wstring> m_comments;
};

struct Scenario {
  std::wstring_view name;
  std::filesystem::path fixturePath;
  std::wstring_view expectedCodePageToken;
  std::wstring_view expectedComments;
  std::wstring_view expectedLayerName;
  std::wstring_view expectedText;
  std::wstring_view expectedMText;
  bool binaryOutput;
  bool expectedWriteSuccess;
};

[[nodiscard]] std::filesystem::path GetFixtureDirectory() {
  return std::filesystem::path{__FILE__}.parent_path().parent_path() / L"TestData" / L"CodePages";
}

void Expect(bool condition, std::wstring_view message, int& failureCount) {
  if (!condition) {
    ++failureCount;
    std::wcerr << L"FAIL: " << message << L'\n';
  }
}

[[nodiscard]] std::wstring MakeMessage(std::wstring_view prefix, std::wstring_view scenarioName) {
  std::wstring message{prefix};
  message += scenarioName;
  return message;
}

[[nodiscard]] std::wstring MakeDiagnosticMessage(
    std::wstring_view prefix, std::wstring_view label, std::wstring_view actual, std::wstring_view expected) {
  std::wstring message{prefix};
  message += label;
  message += L" actual=[";
  message += actual;
  message += L"] expected=[";
  message += expected;
  message += L"]";
  return message;
}

[[nodiscard]] std::wstring DescribeCodeUnits(std::wstring_view text) {
  std::wostringstream stream;
  stream << std::uppercase << std::hex;
  bool firstValue{true};
  for (const auto codeUnit : text) {
    if (!firstValue) { stream << L' '; }
    stream << static_cast<unsigned int>(codeUnit);
    firstValue = false;
  }
  return stream.str();
}

[[nodiscard]] bool ContainsLayer(const RoundTripFixtureModel& model, std::wstring_view layerName) {
  for (const auto& layer : model.Layers()) {
    if (layer.m_tableName == layerName) { return true; }
  }
  return false;
}

[[nodiscard]] std::wstring DescribeLayers(const RoundTripFixtureModel& model) {
  std::wstring description;
  for (const auto& layer : model.Layers()) {
    if (!description.empty()) { description += L", "; }
    description += L"[";
    description += layer.m_tableName;
    description += L"]";
  }
  return description;
}

[[nodiscard]] bool ContainsTextStyle(const RoundTripFixtureModel& model, std::wstring_view styleName, std::wstring_view fontName) {
  for (const auto& textStyle : model.TextStyles()) {
    if (textStyle.m_tableName == styleName && textStyle.font == fontName) { return true; }
  }
  return false;
}

[[nodiscard]] bool ContainsTextValue(const RoundTripFixtureModel& model, std::wstring_view textValue) {
  for (const auto& text : model.Texts()) {
    if (text.m_string == textValue) { return true; }
  }
  return false;
}

[[nodiscard]] bool ContainsMTextValue(const RoundTripFixtureModel& model, std::wstring_view textValue) {
  for (const auto& mText : model.MTexts()) {
    if (mText.m_textString == textValue) { return true; }
  }
  return false;
}

[[nodiscard]] std::wstring DescribeTextValues(const RoundTripFixtureModel& model) {
  std::wstring description;
  for (const auto& text : model.Texts()) {
    if (!description.empty()) { description += L", "; }
    description += L"[";
    description += text.m_string;
    description += L"]";
  }
  return description;
}

[[nodiscard]] std::wstring DescribeMTextValues(const RoundTripFixtureModel& model) {
  std::wstring description;
  for (const auto& mText : model.MTexts()) {
    if (!description.empty()) { description += L", "; }
    description += L"[";
    description += mText.m_textString;
    description += L"]";
  }
  return description;
}

[[nodiscard]] bool ReadFixture(const std::filesystem::path& filePath, RoundTripFixtureModel& model) {
  EoDxfRead reader(filePath);
  return reader.Read(&model, false);
}

void BuildUtf16SourceModel(RoundTripFixtureModel& model) {
  EoDxfHeader header;
  header.AddComment(L"Programmatic UTF-16 comment");
  header.AddWideString(L"$DWGCODEPAGE", L"UTF-16", 3);
  model.addHeader(&header);

  EoDxfClass classData;
  classData.m_classDxfRecordName = L"TEST_CLASS";
  classData.m_cppClassName = L"RoundTripFixtureClass";
  classData.m_applicationName = L"EoDxfLibTests";
  classData.m_proxyCapabilitiesFlag = 0;
  classData.m_instanceCount = 1;
  classData.m_wasAProxyFlag = 0;
  classData.m_isAnEntityFlag = 0;
  model.addClass(classData);

  EoDxfLayer defaultLayer;
  defaultLayer.m_tableName = L"0";
  model.addLayer(defaultLayer);

  EoDxfLayer namedLayer;
  namedLayer.m_tableName = L"LayerUtf16";
  namedLayer.m_linetypeName = L"Continuous";
  namedLayer.m_colorNumber = 7;
  model.addLayer(namedLayer);

  EoDxfTextStyle textStyle;
  textStyle.m_tableName = L"STANDARD";
  textStyle.font = L"txt";
  model.addTextStyle(textStyle);

  EoDxfText text;
  text.m_layer = L"LayerUtf16";
  text.m_textHeight = 2.5;
  text.m_string = L"ASCII only text";
  text.m_textStyleName = L"STANDARD";
  model.addText(text);

  EoDxfMText mText;
  mText.m_layer = L"0";
  mText.m_nominalTextHeight = 2.5;
  mText.m_referenceRectangleWidth = 20.0;
  mText.m_textString = L"MText ASCII only";
  mText.m_textStyleName = L"STANDARD";
  mText.m_attachmentPoint = EoDxfMText::AttachmentPoint::TopLeft;
  model.addMText(mText);
}

[[nodiscard]] std::filesystem::path MakeOutputPath(
    std::wstring_view scenarioName, const std::filesystem::path& fixturePath, bool binaryOutput) {
  auto outputDirectory = std::filesystem::temp_directory_path() / L"EoDxfLibRoundTripTextTests";
  std::filesystem::create_directories(outputDirectory);
  std::wstring fileName{scenarioName};
  fileName += binaryOutput ? L"_binary" : L"_ascii";
  fileName += fixturePath.extension().wstring();
  return outputDirectory / fileName;
}

void RunScenario(const Scenario& scenario, int& failureCount) {
  RoundTripFixtureModel sourceModel;
  if (scenario.fixturePath.empty()) {
    BuildUtf16SourceModel(sourceModel);
  } else {
    Expect(ReadFixture(scenario.fixturePath, sourceModel), MakeMessage(L"Read source fixture failed: ", scenario.name), failureCount);
    if (failureCount != 0 && !std::filesystem::exists(scenario.fixturePath)) { return; }
  }

  if (!ContainsLayer(sourceModel, scenario.expectedLayerName)) {
    auto message = MakeMessage(L"Source layer missing in fixture: ", scenario.name);
    message += L" actual layers=";
    message += DescribeLayers(sourceModel);
    Expect(false, message, failureCount);
    return;
  }

  const auto outputPath = MakeOutputPath(scenario.name, scenario.fixturePath, scenario.binaryOutput);
  std::error_code errorCode;
  std::filesystem::remove(outputPath, errorCode);

  EoDxfWrite writer(outputPath);
  sourceModel.SetWriter(&writer);
  const auto writeSucceeded = writer.Write(&sourceModel, EoDxf::Version::AC1021, scenario.binaryOutput);
  Expect(
      writeSucceeded == scenario.expectedWriteSuccess,
      MakeMessage(L"Write result mismatch: ", scenario.name),
      failureCount);

  if (!scenario.expectedWriteSuccess) {
    Expect(!std::filesystem::exists(outputPath), MakeMessage(L"Rejected write created output: ", scenario.name), failureCount);
    return;
  }

  RoundTripFixtureModel roundTripModel;
  Expect(ReadFixture(outputPath, roundTripModel), MakeMessage(L"Read round-trip output failed: ", scenario.name), failureCount);
  Expect(
      roundTripModel.Header().GetCodePageToken() == scenario.expectedCodePageToken,
      MakeMessage(L"Code page token mismatch: ", scenario.name),
      failureCount);
  Expect(
      roundTripModel.Header().GetComments() == scenario.expectedComments,
      roundTripModel.Header().GetComments() == scenario.expectedComments
          ? MakeMessage(L"Comment mismatch: ", scenario.name)
          : MakeMessage(L"Comment mismatch: ", scenario.name) + std::wstring{L" actual length="} +
                std::to_wstring(roundTripModel.Header().GetComments().size()) + L" actual codes=[" +
                DescribeCodeUnits(roundTripModel.Header().GetComments()) + L"] expected length=" +
                std::to_wstring(scenario.expectedComments.size()) + L" expected codes=[" +
                DescribeCodeUnits(scenario.expectedComments) + L"]",
      failureCount);
  Expect(ContainsLayer(roundTripModel, scenario.expectedLayerName), MakeMessage(L"Layer mismatch: ", scenario.name), failureCount);
  Expect(ContainsTextStyle(roundTripModel, L"STANDARD", L"txt"), MakeMessage(L"Text style mismatch: ", scenario.name), failureCount);
  if (!ContainsTextValue(roundTripModel, scenario.expectedText)) {
    auto message = MakeMessage(L"TEXT mismatch: ", scenario.name);
    message += L" actual texts=";
    message += DescribeTextValues(roundTripModel);
    Expect(false, message, failureCount);
  }
  if (!ContainsMTextValue(roundTripModel, scenario.expectedMText)) {
    auto message = MakeMessage(L"MTEXT mismatch: ", scenario.name);
    message += L" actual mtexts=";
    message += DescribeMTextValues(roundTripModel);
    Expect(false, message, failureCount);
  }
}

void RunNormalizationDiagnostics(int& failureCount) {
  struct NormalizationCase {
    std::wstring_view inputToken;
    std::wstring_view expectedToken;
  };

  const std::vector<NormalizationCase> normalizationCases{
      {L"windows-31j", L"ANSI_932"},
      {L"gb2312", L"ANSI_936"},
      {L"latin-1", L"ANSI_1252"},
      {L"unicode", L"UTF-16"},
      {L"utf8-no-bom", L"UTF-8"},
      {L"unknown-code-page", L"ANSI_1252"},
  };

  for (const auto& normalizationCase : normalizationCases) {
    EoTcTextCodec codec;
    codec.SetCodePage(normalizationCase.inputToken);
    Expect(
        codec.GetCodePage() == normalizationCase.expectedToken,
        MakeDiagnosticMessage(L"Normalization mismatch: ", normalizationCase.inputToken, codec.GetCodePage(), normalizationCase.expectedToken),
        failureCount);
  }
}

void RunBoundaryDiagnostics(int& failureCount) {
  {
    EoTcTextCodec codec;
    codec.SetCodePage(L"UTF-8");
    const std::string invalidUtf8{"\xC3\x28", 2};
    const auto decodedText = codec.DecodeText(invalidUtf8);
    Expect(
        decodedText.empty(),
        MakeDiagnosticMessage(L"Invalid UTF-8 should fail decode: ", L"UTF-8", DescribeCodeUnits(decodedText), L""),
        failureCount);
  }

  {
    EoTcTextCodec codec;
    codec.SetCodePage(L"UTF-16");
    const std::string oddLengthUtf16{"A\0B", 3};
    const auto decodedText = codec.DecodeText(oddLengthUtf16);
    Expect(
        decodedText.empty(),
        MakeDiagnosticMessage(L"Odd-length UTF-16 should fail decode: ", L"UTF-16", DescribeCodeUnits(decodedText), L""),
        failureCount);
  }

  {
    EoTcTextCodec codec;
    codec.SetCodePage(L"UTF-16");
    std::wstring danglingHighSurrogate;
    danglingHighSurrogate.push_back(static_cast<wchar_t>(0xD800));
    const auto encodedText = codec.EncodeText(danglingHighSurrogate);
    Expect(
        encodedText.empty(),
        MakeDiagnosticMessage(L"Dangling UTF-16 high surrogate should fail encode: ", L"UTF-16", L"non-empty result", L""),
        failureCount);
  }

  {
    EoTcTextCodec codec;
    codec.SetCodePage(L"UTF-16");
    const std::string loneLowSurrogate{"\x00\xDC", 2};
    const auto decodedText = codec.DecodeText(loneLowSurrogate);
    Expect(
        decodedText.empty(),
        MakeDiagnosticMessage(L"Lone UTF-16 low surrogate should fail decode: ", L"UTF-16", DescribeCodeUnits(decodedText), L""),
        failureCount);
  }
}

}  // namespace

int wmain() {
  int failureCount{};
  const auto fixtureDirectory = GetFixtureDirectory();

  const std::vector<Scenario> scenarios{
      {L"ansi_1252_ascii", fixtureDirectory / L"ansi_1252_ascii.dxf", L"ANSI_1252", L"Fixture ANSI_1252 caf\u00E9 \u20AC \u2014\nEoDxf 0.1", L"Layer1252", L"caf\u00E9 \u20AC \u2014 na\u00EFve", L"MText caf\u00E9 \u20AC \u2014", false, true},
      {L"ansi_1252_binary", fixtureDirectory / L"ansi_1252_ascii.dxf", L"ANSI_1252", L"Fixture ANSI_1252 caf\u00E9 \u20AC \u2014", L"Layer1252", L"caf\u00E9 \u20AC \u2014 na\u00EFve", L"MText caf\u00E9 \u20AC \u2014", true, true},
      {L"utf8_ascii", fixtureDirectory / L"utf8_ascii.dxf", L"UTF-8", L"Fixture UTF-8 \u041F\u0440\u0438\u0432\u0435\u0442 \u65E5\u672C\u8A9E \u03A9\nEoDxf 0.1", L"LayerUtf8", L"\u041F\u0440\u0438\u0432\u0435\u0442 \u65E5\u672C\u8A9E \u03A9", L"MText \u041F\u0440\u0438\u0432\u0435\u0442 \u65E5\u672C\u8A9E \u03A9", false, true},
      {L"utf8_binary", fixtureDirectory / L"utf8_ascii.dxf", L"UTF-8", L"Fixture UTF-8 \u041F\u0440\u0438\u0432\u0435\u0442 \u65E5\u672C\u8A9E \u03A9", L"LayerUtf8", L"\u041F\u0440\u0438\u0432\u0435\u0442 \u65E5\u672C\u8A9E \u03A9", L"MText \u041F\u0440\u0438\u0432\u0435\u0442 \u65E5\u672C\u8A9E \u03A9", true, true},
      {L"utf16_token_ascii", {}, L"UTF-16", L"Programmatic UTF-16 comment\nEoDxf 0.1", L"LayerUtf16", L"ASCII only text", L"MText ASCII only", false, true},
      {L"utf16_token_binary_rejected", {}, L"UTF-16", L"Programmatic UTF-16 comment", L"LayerUtf16", L"ASCII only text", L"MText ASCII only", true, false},
  };

  for (const auto& scenario : scenarios) {
    RunScenario(scenario, failureCount);
  }

  RunNormalizationDiagnostics(failureCount);
  RunBoundaryDiagnostics(failureCount);

  if (failureCount == 0) {
    std::wcout << L"All EoDxfLib round-trip text tests passed.\n";
    return 0;
  }

  std::wcerr << failureCount << L" EoDxfLib round-trip text test(s) failed.\n";
  return 1;
}
