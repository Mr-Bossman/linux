// SPDX-License-Identifier: GPL-2.0
/*
 * NVMe I/O cancel command implementation.
 * Copyright (c) 2023 Red Hat
 */

#include "nvmet.h"

void nvmet_execute_cancel(struct nvmet_req *req)
{
	u16 cid;
	__le16 sqid;
	bool mult_cmds;
	int ret = 0;
	struct nvmet_ctrl *ctrl = req->sq->ctrl;
	struct nvmet_sq *sq = req->sq;
	struct nvmet_req *treq;
	u32 canceled = 0;


	pr_info("nvmet: CANCEL command_id: 0x%x", req->cmd->cancel.command_id);

	if (!nvmet_check_transfer_len(req, 0)) {
		pr_info("nvmet: CANCEL invalid transfer length");
		return;
	}

	sqid = le16_to_cpu(req->cmd->cancel.sqid);
	if (sqid > ctrl->subsys->max_qid) {
		pr_info("nvmet: CANCEL invalid sqid 0x%x", sqid);
		ret = NVME_SC_INVALID_FIELD | NVME_STATUS_DNR;
		goto exit;
	}

	mult_cmds = req->cmd->cancel.action & NVME_CANCEL_ACTION_MUL_CMD;
	cid  = req->cmd->cancel.cid;

	pr_info("nvmet: CANCEL command_id 0x%x cid 0x%x mult_cmds %d",
			req->cmd->cancel.command_id, cid, mult_cmds);

	if (cid == req->cmd->cancel.command_id && !mult_cmds) {
		/* If action is set to "single command" and cid is
		 * set to the cid of this cancel command, then
		 * the controller shall abort the command with
		 * an "invalid cid" status code.
		 */
		pr_info("nvmet: CANCEL invalid cid 0x%x", cid);
		ret = NVME_SC_INVALID_CID | NVME_STATUS_DNR;
	} else if ((cid != 0xFFFF && mult_cmds) || sqid != sq->qid) {
		/* if action is set to "multiple commands" and
		 * cid isn't set to 0xFFFF, then abort the command
		 * with an "invalid field" status.
		 * if the sqid field doesn't match the sqid of
		 * the queue to which the cancel command is submitted,
		 * then abort the command with an "invalid field" status.
		 */
		pr_info("nvmet: CANCEL invalid field cid 0x%x sqid %d sq->qid %d", cid, sqid, sq->qid);
		ret = NVME_SC_INVALID_FIELD | NVME_STATUS_DNR;
	}

	if (!mult_cmds) {
		treq = xa_load(&sq->outstanding_requests, cid);
		if (treq) {
			if (cancel_delayed_work(&treq->req_work)) {
				pr_info("nvmet: CANCEL success: cid 0x%x", cid);
				nvmet_req_complete(treq, NVME_SC_ABORT_REQ);
				canceled += 1;
			} else {
				pr_info("nvmet: CANCEL failed: cid 0x%x", cid);
			}
		} else {
			pr_info("nvmet: CANCEL request not found: cid 0x%x", cid);
		}
	} else {
		unsigned long ucid;
		xa_for_each(&sq->outstanding_requests, ucid, treq) {
			if (cancel_delayed_work(&treq->req_work)) {
				nvmet_req_complete(treq, NVME_SC_ABORT_REQ);
				canceled += 1;
			}
		}
		pr_info("nvmet: CANCEL removed %d requests", canceled);
	}
exit:
	nvmet_set_result(req, canceled);
	nvmet_req_complete(req, ret);
}

