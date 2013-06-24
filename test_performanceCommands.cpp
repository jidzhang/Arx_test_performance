//
// ObjectARX defined commands

#include "StdAfx.h"
#include "StdArx.h"

const long COUNT = 3500000;

void creat_circle()
{
	//类似circle，line之后的几何体要添加到BlockTable中
	AcDbBlockTable * pBlockTable = NULL;
	AcDbDatabase * pDB = acdbHostApplicationServices()->workingDatabase();
	pDB->getSymbolTable(pBlockTable, AcDb::kForRead);
	AcDbBlockTableRecord * pRecord = NULL;
	pBlockTable->getAt(ACDB_MODEL_SPACE, pRecord, AcDb::kForWrite);
	pBlockTable->close();					//用后及时关闭

	//随机创建COUNT个CIRCLE
	for (long i=0; i<COUNT; ++i) {
		AcDbObjectId id = AcDbObjectId::kNull;
		AcGePoint3d centorPt(0,0,0);		//圆心
		centorPt.x = rand();
		centorPt.y = rand();
		AcGeVector3d normal(0,0,1);			//法线方向
		double radius = rand();				//半径
		AcDbCircle * pCircle = new AcDbCircle(centorPt, normal, radius);
		Adesk::UInt16 colorIndex = rand()%4;
		pCircle->setColorIndex(colorIndex);
		pRecord->appendAcDbEntity(id, pCircle);
		pCircle->close();					//不能用delete
	}
	pRecord->close();						//用后及时关闭
}
//用三种操作数据库的方式改变实体颜色
inline void change_color_method1(AcDbObjectId id)
{
	AcDbEntity * pEnt = NULL;
	if (acdbOpenObject(pEnt, id, AcDb::kForWrite) == Acad::eOk) {
		Adesk::UInt16 colorIndex = 3;
		if (pEnt->colorIndex() != colorIndex) {
			pEnt->setColorIndex(colorIndex);
		}
		pEnt->close();						//用后关闭
	}
}
inline void change_color_method2(AcDbObjectId id)
{
	AcDbEntity * pEnt = NULL;
	if (acdbOpenObject(pEnt, id, AcDb::kForRead) == Acad::eOk) {
		Adesk::UInt16 colorIndex = 3;
		if (pEnt->colorIndex() != colorIndex) {
			pEnt->upgradeOpen();
			pEnt->setColorIndex(colorIndex);
		}
		pEnt->close();						//用后关闭
	}
}
//这个函数在调用前要开启事务
inline void change_color_method3(AcDbObjectId id)
{
	AcDbEntity * pEnt = NULL;
	if (acdbTransactionManager->getObject((AcDbObject*&)pEnt, id, AcDb::kForRead) == Acad::eOk) {
		Adesk::UInt16 colorIndex = 3;
		if (pEnt->colorIndex() != colorIndex) {
			pEnt->upgradeOpen();
			pEnt->setColorIndex(colorIndex);
		}
	}
}

void GetObjectIdArray(const ads_name ssname, AcDbObjectIdArray & ids);
void draw_performance(const CArray<int, int> &idex1, const CArray<UINT, UINT> &time_counter1,
					  const CArray<int, int> &idex2, const CArray<UINT, UINT> &time_counter2,
					  const CArray<int, int> &idex3, const CArray<UINT, UINT> &time_counter3);
void remove_circle(const AcDbObjectIdArray &ids);

// This is command 'TEST'
void test_performance()
{
	srand((unsigned int)time(NULL));
	creat_circle();				//随机创建3000000个circle

	ads_name ssname;
	struct resbuf mask;
	mask.restype = 0;
	mask.resval.rstring = ACRX_T("CIRCLE");
	mask.rbnext = NULL;
	acedSSGet(ACRX_T("A"), NULL, NULL, &mask, ssname);	//获取图面上的所有circle
	AcDbObjectIdArray ids;
	GetObjectIdArray(ssname, ids);
	acedSSFree(ssname);

	//把所有数据分为三部分，然后分别做测试
	int nsize = ids.length();
	assert(nsize > 10);
	nsize /= 3;
	
	AcDbObjectIdArray ids1;		//分为三组
	AcDbObjectIdArray ids2;
	AcDbObjectIdArray ids3;
	int i = 0;
	for (; i<nsize; ++i) {
		ids1.append(ids[i]);
		ids2.append(ids[i+nsize]);
		ids3.append(ids[i+nsize*2]);
	}
	//方式1
	const int DIST = 1000;
	CArray<int, int> idex;
	CArray<UINT, UINT> time_counter1;
	clock_t start, end;
	start = clock();				//test begin
	for (i=0; i<nsize; ++i) {
		change_color_method1(ids1[i]);
		if ((i+1)%DIST==0) {
			idex.Add(i+1);
			end = clock();
			time_counter1.Add(end-start);		//记录每修改1000个使用所用的时间
		}
	}
	idex.Add(nsize+1);
	end = clock();
	time_counter1.Add(end-start);	//test end
	//方式2
	CArray<UINT, UINT> time_counter2;
	start = clock();				//test begin
	for (i=0; i<nsize; ++i) {
		change_color_method2(ids1[i]);
		if ((i+1)%DIST==0) {
			end = clock();
			time_counter2.Add(end-start);
		}
	}
	end = clock();
	time_counter2.Add(end-start);	//test end
	//方式3
	CArray<UINT, UINT> time_counter3;
	start = clock();				//test begin
	acdbTransactionManager->startTransaction();
	for (i=0; i<nsize; ++i) {
		change_color_method3(ids1[i]);
		if ((i+1)%DIST==0) {
			end = clock();
			time_counter3.Add(end-start);
		}
	}
	acdbTransactionManager->endTransaction();
	end = clock();
	time_counter3.Add(end-start);	//test end
	//在AutoCAD中绘制出时间表
	remove_circle(ids);
	ids.removeSubArray(0, ids.length());
	draw_performance(idex, time_counter1,
					 idex, time_counter2,
					 idex, time_counter3);	
}

void GetObjectIdArray(const ads_name ssname, AcDbObjectIdArray & ids)
{
	long len = 0;
	acedSSLength(ssname, &len);
	for(long i=0; i<len; ++i){
		ads_name entres;
		acedSSName(ssname, i, entres);
		AcDbObjectId id;
		if (acdbGetObjectId(id, entres) == Acad::eOk)
			ids.append(id);
	}
}

void draw_performance(const CArray<int, int> &idex1, const CArray<UINT, UINT> &time_counter1,
					  const CArray<int, int> &idex2, const CArray<UINT, UINT> &time_counter2,
					  const CArray<int, int> &idex3, const CArray<UINT, UINT> &time_counter3)
{
	AcGePoint3dArray pts1;
	AcGePoint3dArray pts2;
	AcGePoint3dArray pts3;
	int i = 0 ;
	for (; i<idex1.GetSize(); ++i) {
		pts1.append(AcGePoint3d(i, time_counter1[i], 0));
		pts2.append(AcGePoint3d(i, time_counter2[i], 0));
		pts3.append(AcGePoint3d(i, time_counter3[i], 0));
	}
	AcDb3dPolyline *pLine1 = new AcDb3dPolyline(AcDb::k3dSimplePoly, pts1);
	pLine1->setColorIndex(1);
	pts1.removeSubArray(0, pts1.length());
	AcDb3dPolyline *pLine2 = new AcDb3dPolyline(AcDb::k3dSimplePoly, pts2);
	pLine2->setColorIndex(2);
	pts2.removeSubArray(0, pts2.length());
	AcDb3dPolyline *pLine3 = new AcDb3dPolyline(AcDb::k3dSimplePoly, pts3);
	pLine3->setColorIndex(3);
	pts3.removeSubArray(0, pts3.length());

	AcDbBlockTable * pBlockTable = NULL;
	AcDbDatabase * pDB = acdbHostApplicationServices()->workingDatabase();
	pDB->getSymbolTable(pBlockTable, AcDb::kForRead);
	AcDbBlockTableRecord * pRecord = NULL;
	pBlockTable->getAt(ACDB_MODEL_SPACE, pRecord, AcDb::kForWrite);
	pBlockTable->close();				//用后及时关闭
	
	AcDbObjectId id = AcDbObjectId::kNull;
	pRecord->appendAcDbEntity(id, pLine1);
	pLine1->close();
	pRecord->appendAcDbEntity(id, pLine2);
	pLine2->close();
	pRecord->appendAcDbEntity(id, pLine3);	
	pLine3->close();
	pRecord->close();
}
void remove_circle(const AcDbObjectIdArray &ids)
{
////实体的数量巨大时，这种方式可能导致内存不足
// 	acdbTransactionManager->startTransaction();
// 	for (long i=0; i<ids.length(); ++i) {
// 		AcDbEntity * pEnt = NULL;
// 		if (acdbTransactionManager->getObject((AcDbObject*&)pEnt, ids[i], AcDb::kForWrite) == Acad::eOk) {
// 			pEnt->setVisibility(AcDb::kInvisible);
// 			//pEnt->erase();
// 		}	
// 	}
// 	acdbTransactionManager->endTransaction();

	AcDbEntity * pEnt = NULL;
	for (long i=0; i<ids.length(); ++i) {
		if (acdbOpenObject(pEnt, ids[i], AcDb::kForWrite) == Acad::eOk) {
			pEnt->setVisibility(AcDb::kInvisible);
			pEnt->close();
		}
	}
}