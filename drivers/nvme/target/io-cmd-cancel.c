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

	if (!nvmet_check_transfer_len(req, 0))
		return;

	sqid = le16_to_cpu(req->cmd->cancel.sqid);
	if (sqid > ctrl->subsys->max_qid) {
		ret = NVME_SC_INVALID_FIELD | NVME_STATUS_DNR;
		goto exit;
	}

	mult_cmds = req->cmd->cancel.action & NVME_CANCEL_ACTION_MUL_CMD;
	cid  = req->cmd->cancel.cid;

	if (cid == req->cmd->cancel.command_id && !mult_cmds) {
		/* If action is set to "single command" and cid is
		 * set to the cid of this cancel command, then
		 * the controller shall abort the command with
		 * an "invalid cid" status code.
		 */
		ret = NVME_SC_INVALID_CID | NVME_STATUS_DNR;
	} else if ((cid != 0xFFFF && mult_cmds) || sqid != sq->qid) {
		/* if action is set to "multiple commands" and
		 * cid isn't set to 0xFFFF, then abort the command
		 * with an "invalid field" status.
		 * if the sqid field doesn't match the sqid of
		 * the queue to which the cancel command is submitted,
		 * then abort the command with an "invalid field" status.
		 */
		ret = NVME_SC_INVALID_FIELD | NVME_STATUS_DNR;
	}

exit:
	nvmet_set_result(req, 0);
	nvmet_req_complete(req, ret);
}

