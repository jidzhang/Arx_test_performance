//
// ObjectARX defined commands

#include "StdAfx.h"
#include "StdArx.h"

const long COUNT = 3500000;

void creat_circle()
{
	//����circle��line֮��ļ�����Ҫ��ӵ�BlockTable��
	AcDbBlockTable * pBlockTable = NULL;
	AcDbDatabase * pDB = acdbHostApplicationServices()->workingDatabase();
	pDB->getSymbolTable(pBlockTable, AcDb::kForRead);
	AcDbBlockTableRecord * pRecord = NULL;
	pBlockTable->getAt(ACDB_MODEL_SPACE, pRecord, AcDb::kForWrite);
	pBlockTable->close();					//�ú�ʱ�ر�

	//�������COUNT��CIRCLE
	for (long i=0; i<COUNT; ++i) {
		AcDbObjectId id = AcDbObjectId::kNull;
		AcGePoint3d centorPt(0,0,0);		//Բ��
		centorPt.x = rand();
		centorPt.y = rand();
		AcGeVector3d normal(0,0,1);			//���߷���
		double radius = rand();				//�뾶
		AcDbCircle * pCircle = new AcDbCircle(centorPt, normal, radius);
		Adesk::UInt16 colorIndex = rand()%4;
		pCircle->setColorIndex(colorIndex);
		pRecord->appendAcDbEntity(id, pCircle);
		pCircle->close();					//������delete
	}
	pRecord->close();						//�ú�ʱ�ر�
}
//�����ֲ������ݿ�ķ�ʽ�ı�ʵ����ɫ
inline void change_color_method1(AcDbObjectId id)
{
	AcDbEntity * pEnt = NULL;
	if (acdbOpenObject(pEnt, id, AcDb::kForWrite) == Acad::eOk) {
		Adesk::UInt16 colorIndex = 3;
		if (pEnt->colorIndex() != colorIndex) {
			pEnt->setColorIndex(colorIndex);
		}
		pEnt->close();						//�ú�ر�
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
		pEnt->close();						//�ú�ر�
	}
}
//��������ڵ���ǰҪ��������
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
	creat_circle();				//�������3000000��circle

	ads_name ssname;
	struct resbuf mask;
	mask.restype = 0;
	mask.resval.rstring = ACRX_T("CIRCLE");
	mask.rbnext = NULL;
	acedSSGet(ACRX_T("A"), NULL, NULL, &mask, ssname);	//��ȡͼ���ϵ�����circle
	AcDbObjectIdArray ids;
	GetObjectIdArray(ssname, ids);
	acedSSFree(ssname);

	//���������ݷ�Ϊ�����֣�Ȼ��ֱ�������
	int nsize = ids.length();
	assert(nsize > 10);
	nsize /= 3;
	
	AcDbObjectIdArray ids1;		//��Ϊ����
	AcDbObjectIdArray ids2;
	AcDbObjectIdArray ids3;
	int i = 0;
	for (; i<nsize; ++i) {
		ids1.append(ids[i]);
		ids2.append(ids[i+nsize]);
		ids3.append(ids[i+nsize*2]);
	}
	//��ʽ1
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
			time_counter1.Add(end-start);		//��¼ÿ�޸�1000��ʹ�����õ�ʱ��
		}
	}
	idex.Add(nsize+1);
	end = clock();
	time_counter1.Add(end-start);	//test end
	//��ʽ2
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
	//��ʽ3
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
	//��AutoCAD�л��Ƴ�ʱ���
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
	pBlockTable->close();				//�ú�ʱ�ر�
	
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
////ʵ��������޴�ʱ�����ַ�ʽ���ܵ����ڴ治��
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