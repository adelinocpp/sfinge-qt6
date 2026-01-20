#include <QtTest>
#include "core/density_generator.h"

class TestDensityGenerator : public QObject {
    Q_OBJECT

private slots:
    void testGenerate();
};

void TestDensityGenerator::testGenerate() {
    SFinGe::DensityGenerator generator;
    SFinGe::DensityParameters params;
    params.zoom = 1.0;
    params.amplify = 0.5;
    
    std::vector<float> shapeMap(100 * 100, 1.0f);
    
    generator.setParameters(params);
    generator.setShapeMap(shapeMap, 100, 100);
    
    QImage image = generator.generate();
    
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 100);
    QCOMPARE(image.height(), 100);
}

QTEST_MAIN(TestDensityGenerator)
#include "test_density_generator.moc"
